#!/usr/bin/env python3
"""Camadas do sprite de BATALHA do treinador (visto de costas, 64x64, 5 quadros).

Folhas 64x320 (5 quadros empilhados). Quadro 0 = parado, 1..4 = arremesso da Poke Bola.
Indices semanticos (paleta montada em runtime em src/clothes.c):
 1 contorno | 2/3 pele/sombra | 4/5 cabelo/sombra | 6/7 chapeu/sombra | 8/9 camiseta/sombra
 10/11 jaqueta/sombra | 12/13 calca/sombra | 14 mochila-acessorio | 15 calcado
"""
import os, sys
from PIL import Image, ImageDraw

S, NF = 64, 5
# (ombro->cotovelo, cotovelo->mao) do braco direito (o que joga) em cada quadro
ARM_R = [((41,33),(45,43),(45,53)), ((41,33),(46,41),(48,49)), ((41,33),(46,42),(43,52)),
         ((41,33),(48,32),(47,23)), ((41,33),(49,34),(55,28))]
ARM_L = ((23,33),(20,43),(21,53))

def mask():  return Image.new('L', (S, S), 0)
def poly(m, pts): ImageDraw.Draw(m).polygon(pts, fill=255)
def ell(m, box): ImageDraw.Draw(m).ellipse(box, fill=255)
def rect(m, box): ImageDraw.Draw(m).rectangle(box, fill=255)
def line(m, a, b, w): ImageDraw.Draw(m).line([a, b], fill=255, width=w)
def on(m, x, y): return 0 <= x < S and 0 <= y < S and m.getpixel((x, y)) > 0

class Cv:
    def __init__(s): s.p = [[0]*S for _ in range(S)]
    def put(s, x, y, c):
        if 0 <= x < S and 0 <= y < S: s.p[y][x] = c
    def fill(s, m, main, shade=None, right=2, bottom=2, light=None):
        """pinta a mascara; borda direita/inferior em 'shade' (luz vinda de cima-esquerda)"""
        for y in range(S):
            for x in range(S):
                if not on(m, x, y): continue
                c = main
                if shade is not None and (not on(m, x+right, y) or not on(m, x+1, y+bottom) or not on(m, x+right-1, y+bottom-1)):
                    c = shade
                s.p[y][x] = c
    def mark(s, pts, c):
        for x, y in pts: s.put(x, y, c)
    def hline(s, x0, x1, y, c):
        for x in range(x0, x1+1): s.put(x, y, c)
    def vline(s, x, y0, y1, c):
        for y in range(y0, y1+1): s.put(x, y, c)

def arm_mask(f, width, long_=True, right=True, lo=0.0):
    m = mask()
    sh, el, ha = ARM_R[f] if right else ARM_L
    line(m, sh, el, width); 
    if long_: line(m, el, ha, width-1)
    else:
        mid = ((sh[0]+el[0])//2, (sh[1]+el[1])//2)
        m = mask(); line(m, sh, mid, width)
    return m

def arm_pts(f, right=True):
    return ARM_R[f] if right else ARM_L

# ------------------------------------------------------------------ corpo
def body(f):
    c = Cv()
    m = mask(); ell(m, (24, 8, 40, 26)); c.fill(m, 2, 3, 3, 3)       # cabeca (pele, aparece sob o cabelo)
    m = mask(); rect(m, (28, 24, 36, 32)); c.fill(m, 2, 3, 2, 3)      # pescoco
    for right in (True, False):
        sh, el, ha = arm_pts(f, right)
        m = mask(); line(m, sh, el, 6); line(m, el, ha, 5); c.fill(m, 2, 3, 2, 2)
        m2 = mask(); ell(m2, (ha[0]-3, ha[1]-3, ha[0]+3, ha[1]+3)); c.fill(m2, 2, 3, 2, 2)   # mao
    m = mask(); poly(m, [(25,44),(39,44),(40,58),(24,58)]); c.fill(m, 2, 3, 2, 2)           # pernas (pele)
    return c

# ------------------------------------------------------------------ cabelo
def hair(f, shape):
    c = Cv(); m = mask()
    ell(m, (23, 7, 41, 27)); rect(m, (27, 23, 37, 29))
    if shape == 1:
        for (x, y) in [(23,12),(26,6),(32,3),(38,6),(41,12)]: poly(m, [(x-3,y+6),(x,y-3),(x+3,y+6)])
    elif shape == 2:
        poly(m, [(23,22),(41,22),(43,38),(21,38)]); 
    elif shape == 3:
        line(m, (33,22), (38,38), 5); ell(m, (34,34, 42, 42))
    elif shape == 4:
        poly(m, [(22,19),(42,19),(44,31),(20,31)])
    c.fill(m, 4, 5, 3, 3)
    # mechas
    for x in range(26, 40, 4): c.vline(x, 19, 26 if shape in (0,1,3) else 30, 5)
    c.hline(27, 37, 20, 5)
    return c

# ------------------------------------------------------------------ roupas
def torso_mask(wide=0):
    m = mask(); poly(m, [(22-wide,30),(42+wide,30),(40,46),(24,46)]); return m

def shirt(f, shape):
    c = Cv(); m = torso_mask(); c.fill(m, 8, 9, 3, 3)
    sm = mask()
    for right in (True, False):
        sh, el, ha = arm_pts(f, right)
        if shape == 0:
            mid = ((sh[0]*2+el[0])//3, (sh[1]*2+el[1])//3+1); line(sm, sh, mid, 8)
        else:
            line(sm, sh, el, 8); line(sm, el, ((el[0]*2+ha[0])//3, (el[1]*2+ha[1])//3), 7)
    c.fill(sm, 8, 9, 2, 2)
    for x, y0, y1 in [(30,33,43),(34,34,43)]: c.vline(x, y0, y1, 9)     # dobras
    c.hline(27, 37, 31, 9)                                                  # gola
    return c

def jacket(f, shape):
    c = Cv(); m = mask(); poly(m, [(21,30),(43,30),(41,50),(23,50)]); c.fill(m, 10, 11, 3, 3)
    sm = mask()
    for right in (True, False):
        sh, el, ha = arm_pts(f, right)
        if shape == 0:
            line(sm, sh, el, 9); line(sm, el, ((el[0]*3+ha[0])//4, (el[1]*3+ha[1])//4), 8)
        else:
            mid = ((sh[0]*3+el[0])//4, (sh[1]*3+el[1])//4); line(sm, sh, mid, 5)
    c.fill(sm, 10, 11, 2, 2)
    c.vline(32, 32, 49, 11); c.hline(23, 41, 49, 11); c.hline(26, 38, 31, 11)   # costura, barra, gola
    c.vline(27, 37, 47, 11); c.vline(37, 37, 47, 11)
    return c

def pants(f, shape):
    c = Cv(); m = mask(); poly(m, [(24,44),(40,44),(41,58),(33,58),(32,50),(31,50),(31,58),(23,58)])
    if shape == 1: m = mask(); poly(m, [(24,44),(40,44),(41,53),(33,53),(32,50),(31,50),(31,53),(23,53)])
    c.fill(m, 12, 13, 3, 3)
    c.hline(24, 40, 45, 13)
    c.vline(27, 49, 56 if shape == 0 else 52, 13); c.vline(37, 49, 56 if shape == 0 else 52, 13)
    return c

def shoes(f, shape):
    c = Cv(); top = 57 if shape == 0 else 52
    m = mask(); poly(m, [(23,top),(31,top),(31,63),(21,63),(21,top+4)]); poly(m, [(34,top),(41,top),(46,top+5),(46,63),(34,63)])
    c.fill(m, 15, 1, 2, 2)
    c.hline(21, 46, 62, 1); c.hline(22, 32, top+1, 1) if shape == 1 else None
    c.hline(34, 44, top+2, 1) if shape == 0 else None
    return c

def hat(f, shape):
    c = Cv(); m = mask()
    if shape == 0:
        ell(m, (22, 3, 42, 21)); c.fill(m, 6, 7, 3, 3)
        b = mask(); poly(b, [(38,12),(47,13),(50,17),(40,19)]); c.fill(b, 7, 7, 1, 1)          # aba vista de lado
        c.hline(24, 40, 18, 7); c.vline(32, 4, 18, 7)                                            # costura central
        c.mark([(27,10),(37,10),(28,14),(36,14)], 7)
    elif shape == 1:
        ell(m, (22, 0, 42, 21)); c.fill(m, 6, 7, 3, 3)
        b = mask(); rect(b, (22, 15, 42, 21)); c.fill(b, 7, 6, 3, 2)
        for x in range(24, 41, 3): c.vline(x, 16, 20, 6)
        ell(m, (29, 0, 35, 5)); c.fill(m, 7, 7, 1, 1)
    else:
        ell(m, (23, 1, 41, 16)); c.fill(m, 6, 7, 3, 3)
        b = mask(); ell(b, (12, 11, 52, 21)); c.fill(b, 6, 7, 3, 3)
        c.hline(24, 40, 13, 7)
    return c

def pack(f, shape):
    c = Cv()
    if shape == 0:
        m = mask(); poly(m, [(25,31),(39,31),(41,37),(41,49),(23,49),(23,37)]); c.fill(m, 14, 1, 2, 2)
        c.hline(26, 38, 40, 1); c.vline(32, 32, 39, 1)
        s = mask(); line(s, (27,31), (24,35), 3); line(s, (37,31), (40,35), 3); c.fill(s, 14, 1, 1, 1)
    else:
        s = mask(); line(s, (25,31), (39,45), 3); c.fill(s, 14, 1, 1, 1)
        m = mask(); poly(m, [(38,42),(48,42),(48,52),(38,52)]); c.fill(m, 14, 1, 2, 2)
        c.hline(38, 48, 46, 1)
    return c

def acc(f, shape):
    c = Cv()
    if shape == 0:   # cachecol
        m = mask(); ell(m, (24, 25, 40, 34)); c.fill(m, 14, 1, 2, 2)
        t = mask(); poly(t, [(33,32),(39,32),(40,43),(34,43)]); c.fill(t, 14, 1, 2, 2); c.hline(34, 40, 42, 1)
    else:            # faixa de cabeca
        m = mask(); poly(m, [(23,12),(41,12),(42,17),(22,17)]); c.fill(m, 14, 1, 2, 2)
        c.mark([(26,14),(30,14),(34,14),(38,14)], 1)
    return c

LAYERS = {'body': (body, 1), 'hair': (hair, 5), 'shirt': (shirt, 2), 'jacket': (jacket, 2), 'pants': (pants, 2),
          'shoes': (shoes, 2), 'hat': (hat, 3), 'pack': (pack, 2), 'acc': (acc, 2)}

PAL = [(0,0,0),(32,24,40),(238,190,150),(196,134,104),(90,50,30),(58,30,18),(200,40,40),(130,20,30),
       (60,110,200),(40,80,160),(80,80,90),(48,48,58),(70,90,150),(48,62,110),(220,170,40),(235,235,240)]

def sheet(fn, arg):
    img = Image.new('P', (S, S*NF), 0); px = img.load()
    for f in range(NF):
        cv = fn(f, arg) if arg is not None else fn(f)
        for y in range(S):
            for x in range(S): px[x, f*S+y] = cv.p[y][x]
    return img

def save(img, path):
    flat = []
    for c in PAL: flat += list(c)
    img.putpalette(flat + [0]*(768-len(flat))); img.save(path)

def main(out):
    os.makedirs(out, exist_ok=True)
    for name, (fn, n) in LAYERS.items():
        if name == 'body': save(sheet(fn, None), f'{out}/back_body.png'); continue
        for s in range(n): save(sheet(fn, s), f'{out}/back_{name}_{s}.png')

# ------------------------------------------------------------------ previa
def compose(out, o, f):
    order = ['body']
    for k in ('pants','shoes','shirt','jacket','hair','acc','hat','pack'):
        if o.get(k) is not None: order.append(k)
    cv = [[0]*S for _ in range(S)]
    for k in order:
        fn = 'back_body' if k == 'body' else f'back_{k}_{o[k]}'
        im = Image.open(f'{out}/{fn}.png'); px = im.load()
        for y in range(S):
            for x in range(S):
                v = px[x, f*S+y]
                if v: cv[y][x] = v
    res = [r[:] for r in cv]
    for y in range(S):
        for x in range(S):
            if cv[y][x] == 0 and any(0 <= x+dx < S and 0 <= y+dy < S and cv[y+dy][x+dx] for dx, dy in ((1,0),(-1,0),(0,1),(0,-1))):
                res[y][x] = 1
    return res

def preview(out, path):
    cols = dict(skin=(238,190,150), hair=(90,50,30), hat=(200,40,40), shirt=(60,110,200), jacket=(80,80,90), pants=(70,90,150), gear=(220,170,40), shoes=(60,60,70))
    def pal(o, c):
        p = {0:(0,0,0,0),1:(32,24,40,255)}
        sh = lambda v, k: tuple(int(a*k) for a in v)
        p[2]=c['skin']; p[3]=sh(c['skin'],.82); p[4]=c['hair']; p[5]=sh(c['hair'],.65); p[6]=c['hat']; p[7]=sh(c['hat'],.7)
        p[8]=c['shirt']; p[9]=sh(c['shirt'],.78); p[10]=c['jacket']; p[11]=sh(c['jacket'],.7)
        p[12]=c['pants']; p[13]=sh(c['pants'],.72); p[14]=c['gear']; p[15]=c['shoes']
        return {k: v+(255,) if len(v)==3 else v for k, v in p.items()}
    outs = [dict(hair=0, shirt=0, pants=0, shoes=0, hat=0),
            dict(hair=1, shirt=1, jacket=0, pants=0, shoes=1, pack=0),
            dict(hair=2, shirt=0, pants=1, shoes=0, acc=0, hat=2),
            dict(hair=3, shirt=1, jacket=1, pants=0, shoes=1, pack=1, acc=1)]
    W = Image.new('RGBA', (S*NF, S*len(outs)), (190,205,230,255))
    for r, o in enumerate(outs):
        p = pal(o, cols)
        for f in range(NF):
            c = compose(out, o, f)
            for y in range(S):
                for x in range(S):
                    if c[y][x]: W.putpixel((f*S+x, r*S+y), p[c[y][x]])
    W.resize((W.width*3, W.height*3), Image.NEAREST).save(path)

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else 'graphics/clothes'
    main(out)
    if len(sys.argv) > 2: preview(out, sys.argv[2])
