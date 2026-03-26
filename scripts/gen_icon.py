#!/usr/bin/env python3
"""Generate 32x32 quick access icons for Flip Out.
Draws an upside-down stick figure tilted at 30 degrees."""

import struct
import zlib
import os
import math

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
    """Draw a 32x32 upside-down stick figure tilted 30 degrees."""
    W, H = 32, 32
    pixels = [[(0, 0, 0, 0) for _ in range(W)] for _ in range(H)]

    # Grey/off-white matching Nexus quick access bar icons
    if bright:
        body_col = (235, 235, 235, 255)    # bright white-grey
        head_col = (240, 240, 240, 255)    # slightly brighter head
        motion_col = (200, 200, 200, 140)  # motion lines
    else:
        body_col = (180, 180, 180, 255)    # muted grey
        head_col = (195, 195, 195, 255)    # slightly lighter head
        motion_col = (140, 140, 140, 100)  # subtle motion lines

    def put(x, y, col):
        if 0 <= x < W and 0 <= y < H:
            # Alpha blend for anti-aliasing
            existing = pixels[y][x]
            if existing[3] == 0:
                pixels[y][x] = col
            else:
                # Simple overwrite for now
                pixels[y][x] = col

    def thick_line(x0, y0, x1, y1, col, thickness=2):
        """Draw a line with thickness by offsetting perpendicular."""
        dx = x1 - x0
        dy = y1 - y0
        length = math.sqrt(dx*dx + dy*dy)
        if length == 0:
            return
        # Perpendicular unit vector
        px = -dy / length
        py = dx / length
        for t in range(int(length * 2) + 1):
            frac = t / (length * 2)
            cx = x0 + dx * frac
            cy = y0 + dy * frac
            for offset in range(thickness):
                off = (offset - (thickness - 1) / 2.0) * 0.5
                put(int(round(cx + px * off)), int(round(cy + py * off)), col)

    def filled_circle(cx, cy, r, col):
        for dy in range(-r, r+1):
            for dx in range(-r, r+1):
                if dx*dx + dy*dy <= r*r:
                    put(cx+dx, cy+dy, col)

    def rotate(px, py, angle_deg, cx, cy):
        """Rotate point (px,py) around (cx,cy) by angle_deg."""
        rad = math.radians(angle_deg)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        dx = px - cx
        dy = py - cy
        return (cx + dx * cos_a - dy * sin_a,
                cy + dx * sin_a + dy * cos_a)

    # Define stick figure upside-down (head at bottom) centered at origin
    # then rotate 30 degrees and translate to center of icon
    angle = 30  # degrees clockwise
    center_x, center_y = 16, 16

    # Figure points (relative, upside-down: head bottom, feet top)
    head_y = 7      # head offset from center (below)
    body_top = -1    # top of body (above center)
    body_bot = 5     # bottom of body (near head)
    arm_y = 2        # where arms attach
    arm_spread = 7   # how far arms reach horizontally
    arm_up = -2      # arms angle slightly up
    leg_top_y = -1   # where legs start (top of body)
    leg_end_y = -9   # where legs end (feet, way up)
    leg_spread = 5   # how far legs spread

    def rp(rx, ry):
        """Rotate a relative point around center."""
        return rotate(center_x + rx, center_y + ry, angle, center_x, center_y)

    # Head
    hx, hy = rp(0, head_y)
    filled_circle(int(round(hx)), int(round(hy)), 3, head_col)

    # Body
    bx0, by0 = rp(0, body_top)
    bx1, by1 = rp(0, body_bot)
    thick_line(int(round(bx0)), int(round(by0)), int(round(bx1)), int(round(by1)), body_col, 3)

    # Left arm
    ax0, ay0 = rp(0, arm_y)
    ax1, ay1 = rp(-arm_spread, arm_y + arm_up)
    thick_line(int(round(ax0)), int(round(ay0)), int(round(ax1)), int(round(ay1)), body_col, 2)

    # Right arm
    ax2, ay2 = rp(arm_spread, arm_y + arm_up)
    thick_line(int(round(ax0)), int(round(ay0)), int(round(ax2)), int(round(ay2)), body_col, 2)

    # Left leg
    lx0, ly0 = rp(0, leg_top_y)
    lx1, ly1 = rp(-leg_spread, leg_end_y)
    thick_line(int(round(lx0)), int(round(ly0)), int(round(lx1)), int(round(ly1)), body_col, 2)

    # Right leg
    lx2, ly2 = rp(leg_spread, leg_end_y)
    thick_line(int(round(lx0)), int(round(ly0)), int(round(lx2)), int(round(ly2)), body_col, 2)

    # Motion lines (small dashes near the figure to show movement)
    for i, off_y in enumerate([-3, 0, 3]):
        mx, my = rp(10, off_y)
        put(int(round(mx)), int(round(my)), motion_col)
        put(int(round(mx)) + 1, int(round(my)), motion_col)

    # Add black border: for every non-transparent pixel, fill surrounding
    # transparent pixels with black
    border_col = (0, 0, 0, 255)
    border_pixels = []
    for y in range(H):
        for x in range(W):
            if pixels[y][x][3] > 0:
                for dy in range(-1, 2):
                    for dx in range(-1, 2):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < W and 0 <= ny < H and pixels[ny][nx][3] == 0:
                            border_pixels.append((nx, ny))
    for bx, by in border_pixels:
        pixels[by][bx] = border_col

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
