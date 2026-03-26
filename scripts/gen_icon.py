#!/usr/bin/env python3
"""Generate 32x32 quick access icons for Flip Out.
Draws an upside-down stick figure who just jumped off a trampoline."""

import struct
import zlib
import os

def make_png(pixels, width, height):
    """Create a minimal RGBA PNG from pixel data."""
    def chunk(chunk_type, data):
        c = chunk_type + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

    raw = b''
    for y in range(height):
        raw += b'\x00'  # filter: none
        for x in range(width):
            raw += bytes(pixels[y][x])

    return (b'\x89PNG\r\n\x1a\n' +
            chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) +
            chunk(b'IEND', b''))

def draw_icon(bright=False):
    """Draw a 32x32 upside-down stick figure above a trampoline."""
    W, H = 32, 32
    # transparent background
    pixels = [[(0, 0, 0, 0) for _ in range(W)] for _ in range(H)]

    if bright:
        body_col = (255, 220, 100, 255)   # bright gold
        head_col = (255, 230, 130, 255)    # bright gold head
        tramp_col = (120, 200, 255, 255)   # bright blue trampoline
        leg_col = (200, 220, 100, 255)     # bright legs
    else:
        body_col = (220, 180, 60, 255)     # gold
        head_col = (230, 195, 80, 255)     # gold head
        tramp_col = (80, 160, 220, 255)    # blue trampoline
        leg_col = (220, 180, 60, 255)      # gold legs

    def put(x, y, col):
        if 0 <= x < W and 0 <= y < H:
            pixels[y][x] = col

    def line(x0, y0, x1, y1, col):
        """Bresenham's line."""
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            put(x0, y0, col)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def filled_circle(cx, cy, r, col):
        for dy in range(-r, r+1):
            for dx in range(-r, r+1):
                if dx*dx + dy*dy <= r*r:
                    put(cx+dx, cy+dy, col)

    # === Trampoline at bottom (y=27-28) ===
    # Trampoline surface - curved line
    for x in range(6, 26):
        # slight curve
        offset = int(0.5 + 1.2 * ((x - 16) ** 2) / 100)
        y = 27 + offset
        put(x, y, tramp_col)
        if x >= 8 and x <= 23:
            put(x, y - 1, tramp_col)  # thicker surface

    # Trampoline legs
    line(7, 28, 5, 31, tramp_col)
    line(24, 28, 26, 31, tramp_col)

    # === Upside-down stick figure (head at bottom, feet at top) ===
    # The figure is inverted - they just flipped off the trampoline

    # Head (circle at bottom of figure, around y=22)
    filled_circle(16, 22, 3, head_col)

    # Body (line going up from head)
    line(16, 19, 16, 10, body_col)
    line(15, 19, 15, 10, body_col)  # 2px wide body

    # Arms spread out (from mid-body, angled up-outward since upside down)
    # Left arm - spread out and slightly up
    line(15, 15, 8, 11, body_col)
    line(15, 14, 8, 10, body_col)
    # Right arm
    line(16, 15, 23, 11, body_col)
    line(16, 14, 23, 10, body_col)

    # Legs spread in a V at the top (feet up in the air)
    # Left leg
    line(15, 10, 10, 3, leg_col)
    line(14, 10, 9, 3, leg_col)
    # Right leg
    line(16, 10, 21, 3, leg_col)
    line(17, 10, 22, 3, leg_col)

    # Small motion lines on sides to show movement
    motion_col = (180, 180, 180, 120) if not bright else (220, 220, 220, 160)
    # Left motion lines
    put(5, 14, motion_col)
    put(4, 15, motion_col)
    put(5, 16, motion_col)
    # Right motion lines
    put(26, 14, motion_col)
    put(27, 15, motion_col)
    put(26, 16, motion_col)

    # Upward motion lines above figure
    for dy in range(3):
        put(12, dy, motion_col)
        put(19, dy, motion_col)

    return pixels

def pixels_to_c_array(png_data, name):
    """Convert PNG bytes to C array."""
    lines = []
    lines.append(f"static const unsigned char {name}[] = {{")
    for i in range(0, len(png_data), 16):
        chunk = png_data[i:i+16]
        hex_vals = ', '.join(f'0x{b:02x}' for b in chunk)
        lines.append(f"    {hex_vals},")
    lines.append("};")
    lines.append(f"static const unsigned int {name}_size = {len(png_data)};")
    return '\n'.join(lines)

if __name__ == '__main__':
    # Generate normal icon
    pixels_normal = draw_icon(bright=False)
    png_normal = make_png(pixels_normal, 32, 32)

    # Generate hover icon (brighter)
    pixels_hover = draw_icon(bright=True)
    png_hover = make_png(pixels_hover, 32, 32)

    # Save PNGs for preview
    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_dir = os.path.join(script_dir, '..', 'images')
    os.makedirs(out_dir, exist_ok=True)

    with open(os.path.join(out_dir, 'icon.png'), 'wb') as f:
        f.write(png_normal)
    with open(os.path.join(out_dir, 'icon_hover.png'), 'wb') as f:
        f.write(png_hover)

    # Print C arrays
    print("// Embedded 32x32 stick figure icon (normal)")
    print(pixels_to_c_array(png_normal, "ICON_FLIPOUT"))
    print()
    print("// Embedded 32x32 stick figure icon (hover - brighter)")
    print(pixels_to_c_array(png_hover, "ICON_FLIPOUT_HOVER"))

    print(f"\nPNG files saved to {out_dir}/")
    print(f"Normal: {len(png_normal)} bytes, Hover: {len(png_hover)} bytes")
