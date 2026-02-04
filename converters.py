import fxconv
from PIL import Image

def convert(input, output, params, target):
    recognized = True

    if params["custom-type"] == "celeste-bmp":
        o = convert_celeste_bmp(input, params)
    else:
        recognized = False

    if recognized:
        fxconv.elf(o, output, "_" + params["name"], **target)
        return 0

    return 1

def convert_celeste_bmp(input, params):
    # Convert an indexed bitmap to bopti_image_t while preserving the palette
    img = Image.open(input)
    assert max(idx for (count, idx) in img.getcolors()) <= 15

    imgp = img.getpalette(rawmode='RGB')[:48]
    print("P:", imgp)
    palette = bytes()
    pallen = min(16, len(imgp)//3)
    for i in range(pallen):
        r, g, b = imgp[3*i : 3*(i+1)]
        color = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3)
        palette += fxconv.u16(color)
    for i in range(16 - pallen):
        palette += fxconv.u16(0)

    header = bytes()
    header += fxconv.u8(0x03) #IMAGE_P4_RGB565A
    header += fxconv.u8(0x01 | 0x02) # flags - RO data
    header += fxconv.i16(16) # 16 colors in palette
    header += fxconv.u16(img.width) + fxconv.u16(img.height)
    header += fxconv.i32(0) # stride??

    encoded = bytes()
    assert (img.width % 2) == 0

    for y in range(img.height):
        for x in range(img.width // 2):
            p1 = img.getpixel((2*x, y))
            p2 = img.getpixel((2*x+1, y))
            assert 0 <= p1 <= 15 and 0 <= p2 <= 15
            encoded += bytes([p1*16 + p2])

    return header + palette + encoded
