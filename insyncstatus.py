import os
import sys
import ctypes
from PIL import Image, ImageGrab, ImageStat
from PIL.ImageFilter import FIND_EDGES

ICON_PATH = os.path.join(os.environ['AppData'], 'Insync', 'App', 'res')
SMOOTHNESS_THRESHOLD = 2000

if getattr(sys, 'frozen', False):
    # running in a bundle
    EXEC_DIR = sys._MEIPASS
else:
    # running as a normal Python script
    EXEC_DIR = os.path.dirname(os.path.realpath(__file__))

if sys.maxsize < 2**32:
    c_get_tray_icon_rect = ctypes.cdll.LoadLibrary(r'tray-icon-rect.dll').get_tray_icon_rect
else:
    c_get_tray_icon_rect = ctypes.cdll.LoadLibrary(r'tray-icon-rect.x64.dll').get_tray_icon_rect
c_get_tray_icon_rect.restype = ctypes.c_bool

class TrayIconNotFound(Exception):
    pass

class InsyncChecker():
    def __init__(self):
        """load Insync taskbar icons from AppData folder"""
        try:
            self.iconsRef = {file.name[8:-4].lower(): Image.open(file.path)
                            for file in os.scandir(ICON_PATH)
                            if file.name.startswith('taskbar-') and file.name.endswith('.ico')}
        except FileNotFoundError:
            self.noInsync = True
        else:
            del self.iconsRef['normal']
            del self.iconsRef['offline']
            self.noInsync = False
        try:
            self.mask = Image.open(os.path.join(EXEC_DIR,'mask.png'))
        except FileNotFoundError:
            print('mask.png missing')
            sys.exit(1)
    
    def get_icon(self):
        """return the current Insync tray icon as a PIL.Image"""
        rect = get_tray_icon_rect('Insync')
        return ImageGrab.grab(bbox=rect)
    
    def get_status(self):
        """return the status code of the current Insync tray icon"""
        if self.noInsync:
            return 'Error: Insync not installed'
        try:
            iconActive = self.get_icon()
        except TrayIconNotFound:
            return 'Error: icon not found'
        if measure_corner_smoothness(iconActive) > SMOOTHNESS_THRESHOLD:
            iconActive = iconActive.crop(get_central_rect(iconActive.getbbox()))
            distances = {code: measure_icon_dist(iconActive, iconRef, self.mask)
                        for code, iconRef in self.iconsRef.items()}
            return min(distances, key=distances.get)
        else:
            return 'offline or connecting'

def get_tray_icon_rect(iconText):
    """return the rectangle of the tray icon with text starting with 'iconText,
    if more than one tray icons' text starts with iconText, only the first one is returned'"""
    x1 = ctypes.c_int()
    y1 = ctypes.c_int()
    x2 = ctypes.c_int()
    y2 = ctypes.c_int()
    found = c_get_tray_icon_rect(ctypes.c_wchar_p(iconText), 
        ctypes.byref(x1), ctypes.byref(y1), ctypes.byref(x2), ctypes.byref(y2))
    if not found:
        raise TrayIconNotFound
    return x1.value, y1.value, x2.value, y2.value

def get_central_rect(rect, dx=16, dy=16):
    """Return the (dx,dy)-sized central rectangle of the input `rect`.
    If the either dimension of `rect` is smaller than `dx` or `dy`,  the original dimension is used.
    Default size is (16,16)"""
    xOffset = max((rect[2] - rect[0] - dx) // 2, 0)
    yOffset = max((rect[3] - rect[1] - dy) // 2, 0)
    cRect = (rect[0] + xOffset,
            rect[1] + yOffset,
            rect[0] + xOffset + dx,
            rect[1] + yOffset + dy)
    return cRect

def measure_icon_dist(img1, img2, mask=None):
    """Measure distance between two images with a mask"""
    if not mask:
        mask = Image.new('1', img1.size, 255)
    return sum(map(measure_pix_dist, img1.getdata(), img2.getdata(), mask.getdata()))

def measure_pix_dist(pix1, pix2, m):
    """Measure distance between two pixels masked by m"""
    dist = lambda x, y: abs(x-y)*(m>0)
    return sum(map(dist, pix1, pix2))

def measure_corner_smoothness(img):
    """measure the smoothness of the lower left corner where there's nothing when Insync not connected"""
    oRect = img.getbbox()
    iRect = get_central_rect(oRect)
    tRect = (1, iRect[1]+11, iRect[0]+9, oRect[3]-1)
    return sum(ImageStat.Stat(img.filter(FIND_EDGES).crop(box=tRect)).sum)

if __name__ == '__main__':
    insyncChecker = InsyncChecker()
    print(insyncChecker.get_status())
