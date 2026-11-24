#ifndef GUARD_CONSTANTS_CLOTHES_H
#define GUARD_CONSTANTS_CLOTHES_H

// Categorias do sistema de roupas (uma peca equipada por categoria)
#define CLOTHES_CAT_BODY        0
#define CLOTHES_CAT_SKIN        1
#define CLOTHES_CAT_HAIR        2
#define CLOTHES_CAT_HAIRCOLOR   3
#define CLOTHES_CAT_SHIRT       4
#define CLOTHES_CAT_JACKET      5
#define CLOTHES_CAT_PANTS       6
#define CLOTHES_CAT_SHOES       7
#define CLOTHES_CAT_CAP         8
#define CLOTHES_CAT_HAT         9
#define CLOTHES_CAT_GLASSES     10
#define CLOTHES_CAT_PACK        11
#define CLOTHES_CAT_ACC         12
#define CLOTHES_CAT_COUNT       13

#define CLOTHES_MAX_ITEMS       128   // limite do bitfield de posse no save
#define CLOTHES_NUM_FRAMES      20    // quadros da folha do jogador (andar + correr + balancar a cabeca)
#define CLOTHES_FRAME_SIZE      256   // 16x32 px em 4bpp

#define CLOTHES_SHAPE_NONE      0xFF

// Lojas (bitmask usado em ClothesItem.shops)
#define CLOTHES_SHOP_PALLET     0
#define CLOTHES_SHOP_CELADON    1
#define CLOTHES_SHOPMASK_PALLET   (1 << CLOTHES_SHOP_PALLET)
#define CLOTHES_SHOPMASK_CELADON  (1 << CLOTHES_SHOP_CELADON)
#define CLOTHES_SHOP_CLOSET     0xFF  // armario pessoal (sem loja)

#define CLOTHES_SAVE_MAGIC      0xC7
#define CLOTHES_PAL_TAG         0x1120 // tag de paleta de objeto do jogador personalizado

#endif // GUARD_CONSTANTS_CLOTHES_H
