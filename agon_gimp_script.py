
#!/usr/bin/env python

# GIMP Python-Fu script to create an image with the AGON VDP 64-color palette

from gimpfu import *

def agon_create_image(width, height):
    """
    Creates a new GIMP image with the AGON VDP 64-color palette.
    """
    # AGON palette (from agon_palette.h)
    agon_palette = [
        (0, 0, 0), (0, 0, 85), (0, 0, 170), (0, 0, 255),
        (0, 85, 0), (0, 85, 85), (0, 85, 170), (0, 85, 255),
        (0, 170, 0), (0, 170, 85), (0, 170, 170), (0, 170, 255),
        (0, 255, 0), (0, 255, 85), (0, 255, 170), (0, 255, 255),
        (85, 0, 0), (85, 0, 85), (85, 0, 170), (85, 0, 255),
        (85, 85, 0), (85, 85, 85), (85, 85, 170), (85, 85, 255),
        (85, 170, 0), (85, 170, 85), (85, 170, 170), (85, 170, 255),
        (85, 255, 0), (85, 255, 85), (85, 255, 170), (85, 255, 255),
        (170, 0, 0), (170, 0, 85), (170, 0, 170), (170, 0, 255),
        (170, 85, 0), (170, 85, 85), (170, 85, 170), (170, 85, 255),
        (170, 170, 0), (170, 170, 85), (170, 170, 170), (170, 170, 255),
        (170, 255, 0), (170, 255, 85), (170, 255, 170), (170, 255, 255),
        (255, 0, 0), (255, 0, 85), (255, 0, 170), (255, 0, 255),
        (255, 85, 0), (255, 85, 85), (255, 85, 170), (255, 85, 255),
        (255, 170, 0), (255, 170, 85), (255, 170, 170), (255, 170, 255),
        (255, 255, 0), (255, 255, 85), (255, 255, 170), (255, 255, 255)
    ]

    # Create a new palette in GIMP
    palette_name = "AGON-64"
    pdb.gimp_palette_new(palette_name)
    for r, g, b in agon_palette:
        pdb.gimp_palette_add_entry(palette_name, "", (r, g, b))

    # Create a new image
    image = gimp.Image(width, height, RGB)
    gimp.Display(image)

    # Convert the image to indexed color mode using the new palette
    pdb.gimp_image_convert_indexed(image, NO_DITHER, MAKE_PALETTE, 64, False, False, palette_name)
    
    # Refresh the display
    gimp.displays_flush()

register(
    "python_fu_agon_create_image",
    "Create AGON VDP Image",
    "Creates a new image with the AGON VDP 64-color palette.",
    "Your Name",
    "Your Name",
    "2025",
    "<Image>/File/Create/AGON Image...",
    "",
    [
        (PF_INT, "width", "Width", 320),
        (PF_INT, "height", "Height", 240)
    ],
    [],
    agon_create_image)

main()
