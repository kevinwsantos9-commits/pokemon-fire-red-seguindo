#!/usr/bin/env python3
"""Pre-visualizacao fiel ao compositor do jogo (mesma ordem de camadas + contorno)."""
import sys
from PIL import Image
sys.path.insert(0, __file__.rsplit('/', 1)[0])
from gen_layers import W, H, NF, FRAMES

PAL = {0:(0,0,0,0),1:(24,20,36,255),8:(150,110,20,255),15:(50,50,60,255)}
def pal_for(o):
    p = dict(PAL)
    C = o['c']; sk = C['skin']; p[2]=sk; p[3]=tuple(int(v*.82) for v in sk)
    p[4]=C['hair']; p[5]=tuple(int(v*.7) for v in C['hair'])
    p[6]=C['hat']; p[7]=tuple(int(v*.7) for v in C['hat'])
    p[9]=C['shirt']; p[10]=tuple(int(v*.78) for v in C['shirt'])
    p[11]=C['jacket']; p[12]=tuple(int(v*.7) for v in C['jacket'])
    p[13]=C['pants']; p[14]=C['gear']; p[8]=tuple(int(v*.72) for v in C['pants']); p[15]=C['shoes']
    return {k:(v+(255,) if len(v)==3 else v) for k,v in p.items()}

def load(name):
    im = Image.open(f'graphics/clothes/{name}.png'); return im, im.load()

def compose(o, f):
    cv = [[0]*W for _ in range(H)]
    d = FRAMES[f][0]
    order = []
    if o.get('pack') is not None and d != 'N': order.append(('pack', o['pack']))
    order.append(('body_f' if o['female'] else 'body_m', None))
    for k in ('pants','shoes','shirt','jacket','hair','glasses','acc','hat'):
        if o.get(k) is not None: order.append((k, o[k]))
    if o.get('pack') is not None and d == 'N': order.append(('pack', o['pack']))
    for name, s in order:
        fn = name if s is None else f'{name}_{s}'
        im, px = load(fn)
        for y in range(H):
            for x in range(W):
                v = px[f*W+x, y]
                if v: cv[y][x] = v
    out = [row[:] for row in cv]
    for y in range(H):
        for x in range(W):
            if cv[y][x] == 0:
                for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    xx, yy = x+dx, y+dy
                    if 0 <= xx < W and 0 <= yy < H and cv[yy][xx]:
                        out[y][x] = 1; break
    return out

def render(o, frames, scale=6):
    p = pal_for(o)
    img = Image.new('RGBA', (W*len(frames), H), (190,205,230,255))
    for i, f in enumerate(frames):
        c = compose(o, f)
        for y in range(H):
            for x in range(W):
                if c[y][x]: img.putpixel((i*W+x, y), p[c[y][x]])
    return img.resize((img.width*scale, img.height*scale), Image.NEAREST)

if __name__ == '__main__':
    cols = dict(skin=(238,190,150), hair=(90,50,30), hat=(200,40,40), shirt=(60,110,200),
                jacket=(80,80,90), pants=(70,90,150), gear=(220,170,40), shoes=(60,60,70))
    base = dict(female=False, c=cols)
    outs = [
      dict(base, hair=0, shirt=0, pants=0, shoes=0),
      dict(base, hair=1, shirt=1, jacket=0, pants=0, shoes=1, hat=0, pack=0),
      dict(base, female=True, hair=2, shirt=0, pants=1, shoes=0, glasses=0, acc=0),
      dict(base, female=True, hair=3, shirt=1, jacket=1, pants=0, shoes=1, hat=2, pack=1, glasses=1),
      dict(base, hair=4, shirt=0, pants=1, shoes=0, hat=1, acc=1),
    ]
    frames = [0,1,2,3,4,5,7,10,13,16]
    rows = [render(o, frames) for o in outs]
    sheet = Image.new('RGBA', (rows[0].width, sum(r.height for r in rows)))
    y = 0
    for r in rows: sheet.paste(r, (0, y)); y += r.height
    sheet.save(sys.argv[1] if len(sys.argv) > 1 else '/tmp/preview.png')
