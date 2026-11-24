#!/usr/bin/env python3
"""Gera as camadas de sprite do sistema de roupas (graphics/clothes/*.png).

Cada camada e uma folha de 20 quadros 16x32 (320x32), 4bpp indexado.
Indices semanticos de paleta (a paleta real e montada em runtime):
 0 transparente | 1 contorno | 2/3 pele/sombra | 4/5 cabelo/sombra
 6/7 chapeu/sombra | 8 calca-sombra | 9/10 camiseta/sombra
 11/12 jaqueta/sombra | 13 calca | 14 acessorio | 15 calcado
Quadros: 0-8 andar (S,N,O parado + 2 passos cada), 9-17 correr (mesma ordem),
18-19 balancar cabeca (sul).  Direcao leste = espelho do oeste (feito pelo hardware).
"""
import os, sys
from PIL import Image

W, H, NF = 16, 32, 20
# (direcao, passo, corrida) por quadro
FRAMES = []
for run in (0, 1):
    for d in 'SNO':
        for st in (0, 1, 2):
            FRAMES.append((d, st, run))
FRAMES += [('S', 1, 0), ('S', 2, 0)]
# ordem real dos quadros: 0..2 = parado S,N,O ; 3,4 = S passos ; 5,6 = N ; 7,8 = O
ORDER = [('S',0),('N',0),('O',0),('S',1),('S',2),('N',1),('N',2),('O',1),('O',2)]
FRAMES = [(d, st, 0) for d, st in ORDER] + \
         [(d, st, 1) for d, st in ORDER] + [('S', 1, 0), ('S', 2, 0)]
# quadros 9..17 correr: mesma ordem (base S,N,O, passos...) -> anim usa 9(S) 10,11(S passos), 12(N) 13,14, 15(O) 16,17
FRAMES = [('S',0,0),('N',0,0),('O',0,0),('S',1,0),('S',2,0),('N',1,0),('N',2,0),('O',1,0),('O',2,0),
          ('S',0,1),('S',1,1),('S',2,1),('N',0,1),('N',1,1),('N',2,1),('O',0,1),('O',1,1),('O',2,1),
          ('S',1,0),('S',2,0)]
assert len(FRAMES) == NF

class Cv:
    def __init__(s): s.p = [[0]*W for _ in range(H)]
    def put(s, x, y, c):
        if 0 <= x < W and 0 <= y < H: s.p[y][x] = c
    def rect(s, x0, y0, x1, y1, c):
        for y in range(y0, y1+1):
            for x in range(x0, x1+1): s.put(x, y, c)
    def mark(s, pts, c):
        for x, y in pts: s.put(x, y, c)
    def vline(s, x, y0, y1, c):
        for y in range(y0, y1+1): s.put(x, y, c)

class Pose:
    def __init__(s, d, st, run):
        s.d, s.st, s.run = d, st, run
        s.bob = 1 if st else 0
        if run: s.bob = 1
        s.y0 = 12 + s.bob
    def r(s, n): return s.y0 + n
    def lifted(s):  # (esq, dir) perna levantada
        if s.st == 1: return (True, False)
        if s.st == 2: return (False, True)
        return (False, False)
    def armdy(s):   # deslocamento vertical dos bracos em S/N
        a = 1 if s.run else 1
        if s.st == 1: return (a, 0)
        if s.st == 2: return (0, a)
        return (0, 0)
    def armdx(s):   # vista lateral
        if s.st == 1: return -1 if not s.run else -2
        if s.st == 2: return 1 if not s.run else 2
        return 0

def legs_xy(P):
    """retorna lista de (x0,x1,ultima_linha_relativa) das pernas por direcao"""
    d = P.d; lf = P.lifted()
    if d in 'SN':
        return [(4,6,17 if lf[0] else 18), (9,11,17 if lf[1] else 18)]
    # lateral: perna da frente e de tras
    if P.st == 0:   return [(6,8,18),(7,9,18)]
    if P.st == 1:   return [(4,6,17),(8,10,18)]
    return [(8,10,17),(5,7,18)]

# ---------------------------------------------------------------- corpo
def body(P, female):
    c = Cv(); r = P.r; d = P.d
    if d in 'SN':
        c.rect(4, r(1), 11, r(7), 2); c.rect(5, r(8), 10, r(8), 2)
        c.rect(3, r(5), 3, r(6), 2); c.rect(12, r(5), 12, r(6), 2)
        c.rect(7, r(9), 8, r(9), 3)
        c.rect(4, r(9), 11, r(14), 2)
        adl, adr = P.armdy()
        c.rect(2, r(9)+adl, 3, r(14)+adl, 2); c.rect(12, r(9)+adr, 13, r(14)+adr, 2)
        if d == 'S':
            c.put(5, r(5), 1); c.put(5, r(6), 1); c.put(10, r(5), 1); c.put(10, r(6), 1)
            if female: c.put(4, r(4), 1); c.put(11, r(4), 1)
            c.put(7, r(7), 3); c.put(8, r(7), 3); c.put(8, r(6), 3)
            c.put(4, r(7), 3); c.put(11, r(7), 3)
    else:
        c.rect(3, r(1), 10, r(7), 2); c.rect(4, r(8), 9, r(8), 2)
        c.put(2, r(5), 2)                       # nariz
        c.put(4, r(5), 1); c.put(4, r(6), 1)    # olho
        c.put(3, r(7), 3)
        if female: c.put(3, r(4), 1)
        c.put(8, r(5), 3); c.put(8, r(6), 3)    # orelha
        c.rect(6, r(9), 7, r(9), 3)
        c.rect(5, r(9), 10, r(14), 2)
        dx = P.armdx()
        c.rect(6+dx, r(9), 8+dx, r(14), 3)
        c.rect(6+dx, r(14), 8+dx, r(14), 2)
    for i, (x0, x1, last) in enumerate(legs_xy(P)):
        c.rect(x0, r(15), x1, r(last), 2 if (d in 'SN' or i == 0) else 3)
    return c

# ---------------------------------------------------------------- cabelo
def hair(P, shape):
    c = Cv(); r = P.r; d = P.d
    if d == 'S':
        c.rect(5, r(0), 10, r(0), 4); c.rect(4, r(1), 11, r(1), 4); c.rect(3, r(2), 12, r(2), 4)
        c.rect(3, r(3), 4, r(3), 4); c.rect(11, r(3), 12, r(3), 4)
        c.rect(5, r(3), 6, r(3), 4); c.rect(9, r(3), 10, r(3), 4)
        c.put(6, r(4), 5); c.put(9, r(4), 5); c.rect(3, r(4), 3, r(4), 5); c.rect(12, r(4), 12, r(4), 5)
        c.mark([(6, r(1)), (9, r(1)), (4, r(2)), (11, r(2)), (7, r(0))], 5)
        if shape == 1:  # espetado
            for x in (4, 7, 10): c.put(x, r(-1), 4)
            c.put(5, r(-1), 5); c.put(9, r(-1), 5); c.rect(3, r(1), 3, r(1), 4); c.rect(12, r(1), 12, r(1), 4)
        elif shape == 2:  # longo
            c.rect(3, r(5), 3, r(10), 4); c.rect(12, r(5), 12, r(10), 4); c.rect(3, r(11), 3, r(11), 5); c.rect(12, r(11), 12, r(11), 5)
        elif shape == 3:  # rabo
            c.rect(12, r(4), 13, r(8), 4); c.put(13, r(9), 5)
        elif shape == 4:  # chanel
            c.rect(3, r(5), 3, r(7), 4); c.rect(12, r(5), 12, r(7), 4); c.put(3, r(8), 5); c.put(12, r(8), 5)
    elif d == 'N':
        c.rect(5, r(0), 10, r(0), 4); c.rect(4, r(1), 11, r(1), 4); c.rect(3, r(2), 12, r(7), 4)
        c.rect(4, r(8), 11, r(8), 4); c.rect(3, r(7), 12, r(7), 5)
        c.rect(5, r(8), 10, r(8), 5)
        c.mark([(5, r(3)), (7, r(4)), (9, r(3)), (6, r(5)), (8, r(5)), (10, r(5)), (7, r(6))], 5)
        if shape == 1:
            for x in (4, 7, 10): c.put(x, r(-1), 4)
            c.put(5, r(-1), 5); c.put(9, r(-1), 5)
        elif shape == 2:
            c.rect(4, r(9), 11, r(12), 4); c.rect(4, r(12), 11, r(12), 5)
        elif shape == 3:
            c.rect(7, r(8), 8, r(13), 4); c.rect(7, r(13), 8, r(13), 5)
        elif shape == 4:
            c.rect(3, r(8), 12, r(9), 4); c.rect(3, r(9), 12, r(9), 5)
    else:  # O
        c.rect(5, r(0), 10, r(0), 4); c.rect(4, r(1), 11, r(1), 4); c.rect(3, r(2), 11, r(2), 4)
        c.rect(8, r(3), 11, r(8), 4); c.rect(3, r(3), 7, r(3), 4)
        c.rect(7, r(4), 8, r(4), 4); c.rect(9, r(8), 11, r(8), 5); c.put(3, r(3), 5)
        c.mark([(9, r(3)), (10, r(5)), (9, r(6)), (6, r(1)), (8, r(2))], 5)
        if shape == 1:
            for x in (4, 7, 10): c.put(x, r(-1), 4)
            c.put(5, r(-1), 5); c.put(9, r(-1), 5)
        elif shape == 2:
            c.rect(8, r(9), 11, r(12), 4); c.rect(8, r(12), 11, r(12), 5)
        elif shape == 3:
            c.rect(11, r(4), 13, r(9), 4); c.rect(12, r(10), 13, r(10), 5)
        elif shape == 4:
            c.rect(8, r(9), 11, r(10), 4); c.rect(8, r(10), 11, r(10), 5)
    return c

# ---------------------------------------------------------------- torso
def torso_box(P):
    return (4, 11) if P.d in 'SN' else (5, 10)

def sleeves(c, P, col, shade, long_):
    r = P.r
    if P.d in 'SN':
        adl, adr = P.armdy(); end = 13 if long_ else 10
        c.rect(2, r(9)+adl, 3, r(end)+adl, col); c.rect(12, r(9)+adr, 13, r(end)+adr, col)
        c.rect(2, r(end)+adl, 3, r(end)+adl, shade); c.rect(12, r(end)+adr, 13, r(end)+adr, shade)
    else:
        dx = P.armdx(); end = 13 if long_ else 10
        c.rect(6+dx, r(9), 8+dx, r(end), col); c.rect(6+dx, r(end), 8+dx, r(end), shade)

def shirt(P, shape):
    c = Cv(); r = P.r; x0, x1 = torso_box(P)
    c.rect(x0, r(9), x1, r(14), 9); c.rect(x0, r(14), x1, r(14), 10)
    c.rect(x1, r(10), x1, r(13), 10)
    if P.d == 'S':
        c.rect(7, r(9), 8, r(9), 10); c.mark([(6, r(9)), (9, r(9)), (7, r(10)), (8, r(10))], 10)
        c.mark([(5, r(12)), (10, r(12)), (6, r(13))], 10)
    elif P.d == 'N': c.mark([(6, r(11)), (9, r(12))], 10)
    sleeves(c, P, 9, 10, shape == 1)
    return c

def jacket(P, shape):
    c = Cv(); r = P.r; x0, x1 = torso_box(P)
    c.rect(x0, r(9), x1, r(14), 11); c.rect(x0, r(14), x1, r(14), 12)
    c.rect(x1, r(10), x1, r(13), 12)
    if P.d == 'S':
        c.rect(7, r(10), 8, r(14), 0)           # abertura (mostra camiseta)
        c.rect(6, r(10), 6, r(14), 12); c.rect(9, r(10), 9, r(14), 12)
        c.rect(7, r(9), 8, r(9), 12)
    elif P.d == 'O':
        c.rect(5, r(10), 5, r(14), 12)
    if shape == 0: sleeves(c, P, 11, 12, True)
    else:
        sleeves(c, P, 11, 12, False)   # colete = mangas curtas escuras
    return c

def pants(P, shape):
    c = Cv(); r = P.r; d = P.d
    x0, x1 = torso_box(P)
    c.rect(x0, r(14), x1, r(14), 13)
    for i, (a, b, last) in enumerate(legs_xy(P)):
        end = 16 if shape == 0 else 15
        end = min(end, last)
        c.rect(a, r(15), b, r(end), 13)
        c.rect(b, r(15), b, r(end), 8)
    c.rect(x0, r(14), x1, r(14), 8) if False else None
    if d in 'SN':
        c.put(7, r(15), 8); c.put(8, r(15), 8)
    return c

def shoes(P, shape):
    c = Cv(); r = P.r; d = P.d
    for i, (a, b, last) in enumerate(legs_xy(P)):
        top = last - 1 if shape == 0 else last - 2
        c.rect(a, r(top), b, r(last), 15)
        if shape == 1: c.rect(a, r(top), b, r(top), 15)
        if d == 'O' and shape == 0 and i == 0: c.put(a-1, r(last), 15)
    return c

# ---------------------------------------------------------------- cabeca
def hat(P, shape):
    c = Cv(); r = P.r; d = P.d
    if shape == 0:   # bone
        if d == 'S':
            c.rect(5, r(-1), 10, r(-1), 6); c.rect(4, r(0), 11, r(2), 6); c.rect(3, r(2), 12, r(2), 7)
            c.rect(4, r(3), 11, r(3), 7); c.rect(5, r(-1), 6, r(-1), 6); c.put(10, r(1), 7)
            c.put(7, r(-1), 7); c.put(8, r(0), 7); c.put(7, r(0), 6); c.mark([(10, r(0)), (11, r(1))], 7)
        elif d == 'N':
            c.rect(5, r(-1), 10, r(-1), 6); c.rect(3, r(0), 12, r(4), 6); c.rect(3, r(4), 12, r(4), 7); c.rect(5, r(5), 10, r(5), 7)
            c.vline(7, r(0), r(3), 7); c.vline(8, r(0), r(3), 7); c.mark([(5, r(2)), (10, r(2))], 7)
        else:
            c.rect(5, r(-1), 10, r(-1), 6); c.rect(3, r(0), 11, r(3), 6); c.rect(2, r(3), 6, r(3), 7)
            c.rect(11, r(2), 11, r(4), 7); c.rect(1, r(3), 3, r(4), 7)
    elif shape == 1:  # gorro
        if d == 'S':
            c.rect(5, r(-2), 10, r(-2), 6); c.rect(4, r(-1), 11, r(2), 6); c.rect(3, r(2), 12, r(3), 7)
        elif d == 'N':
            c.rect(5, r(-2), 10, r(-2), 6); c.rect(3, r(-1), 12, r(4), 6); c.rect(3, r(3), 12, r(4), 7)
        else:
            c.rect(5, r(-2), 10, r(-2), 6); c.rect(3, r(-1), 11, r(2), 6); c.rect(3, r(2), 11, r(3), 7)
    else:             # chapeu de aba larga
        c.rect(5, r(-2), 10, r(-1), 6); c.rect(4, r(0), 11, r(1), 6)
        c.rect(1, r(2), 14, r(2), 7); c.rect(2, r(3), 13, r(3), 7) if d != 'N' else c.rect(2, r(3), 13, r(3), 7)
    return c

def glasses(P, shape):
    c = Cv(); r = P.r; d = P.d
    if d == 'N': return c
    if d == 'S':
        if shape == 0:
            for x in (5, 6, 9, 10): c.put(x, r(4), 1); c.put(x, r(6), 1)
            for x in (4, 7, 8, 11): c.put(x, r(5), 1)
            c.put(6, r(5), 1); c.put(9, r(5), 1)
        else:
            c.rect(4, r(5), 11, r(5), 1); c.rect(5, r(6), 6, r(6), 1); c.rect(9, r(6), 10, r(6), 1)
    else:
        if shape == 0:
            c.rect(3, r(4), 5, r(4), 1); c.rect(3, r(6), 5, r(6), 1); c.put(2, r(5), 1); c.put(5, r(5), 1); c.rect(6, r(5), 8, r(5), 1)
        else:
            c.rect(2, r(5), 8, r(5), 1); c.rect(3, r(6), 5, r(6), 1)
    return c

def pack(P, shape):
    c = Cv(); r = P.r; d = P.d
    if d == 'S':
        if shape == 0:
            c.rect(5, r(9), 5, r(13), 14); c.rect(10, r(9), 10, r(13), 14)
            c.rect(3, r(10), 3, r(15), 14); c.rect(12, r(10), 12, r(15), 14)
        else:
            for i in range(6): c.put(5+i, r(9)+i, 14)
            c.rect(11, r(12), 13, r(15), 14); c.rect(11, r(15), 13, r(15), 1)
    elif d == 'N':
        if shape == 0:
            c.rect(3, r(9), 12, r(15), 14); c.rect(3, r(15), 12, r(15), 8); c.rect(5, r(12), 10, r(12), 1)
        else:
            for i in range(6): c.put(10-i, r(9)+i, 14)
            c.rect(2, r(12), 4, r(15), 14); c.rect(2, r(15), 4, r(15), 1)
    else:
        if shape == 0:
            c.rect(9, r(9), 12, r(15), 14); c.rect(9, r(15), 12, r(15), 1)
        else:
            for i in range(5): c.put(6+i, r(9)+i, 14)
            c.rect(9, r(13), 11, r(16), 14); c.rect(9, r(16), 11, r(16), 1)
    return c

def acc(P, shape):
    c = Cv(); r = P.r; d = P.d
    if shape == 0:  # cachecol
        if d in 'SN':
            c.rect(4, r(9), 11, r(9), 14)
            if d == 'S': c.rect(5, r(10), 6, r(12), 14); c.rect(5, r(12), 6, r(12), 1)
        else:
            c.rect(5, r(9), 10, r(9), 14); c.rect(9, r(10), 10, r(12), 14); c.rect(9, r(12), 10, r(12), 1)
    else:           # faixa de cabeca
        if d == 'O': c.rect(3, r(2), 11, r(2), 14)
        else: c.rect(3, r(2), 12, r(2), 14)
        if d == 'N': c.put(13, r(3), 14); c.put(13, r(4), 1)
    return c

# ---------------------------------------------------------------- saida
def sheet(fn):
    img = Image.new('P', (W*NF, H), 0)
    px = img.load()
    for f, (d, st, run) in enumerate(FRAMES):
        cv = fn(Pose(d, st, run))
        for y in range(H):
            for x in range(W):
                px[f*W+x, y] = cv.p[y][x]
    return img

PAL = [(0,0,0),(32,24,40),(238,190,150),(200,140,110),(90,50,30),(60,32,20),(200,40,40),(140,20,30),
       (120,120,140),(60,110,200),(40,80,160),(80,80,90),(50,50,60),(70,90,150),(220,170,40),(60,60,70)]
def save(img, path):
    flat = []
    for c in PAL: flat += list(c)
    img.putpalette(flat + [0]*(768-len(flat)))
    img.save(path)

def main(out):
    os.makedirs(out, exist_ok=True)
    save(sheet(lambda P: body(P, False)), f'{out}/body_m.png')
    save(sheet(lambda P: body(P, True)),  f'{out}/body_f.png')
    spec = {'hair':(hair,5),'shirt':(shirt,2),'jacket':(jacket,2),'pants':(pants,2),'shoes':(shoes,2),
            'hat':(hat,3),'glasses':(glasses,2),'pack':(pack,2),'acc':(acc,2)}
    for name, (fn, n) in spec.items():
        for s in range(n):
            save(sheet(lambda P, fn=fn, s=s: fn(P, s)), f'{out}/{name}_{s}.png')

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'graphics/clothes')
