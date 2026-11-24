#ifndef GUARD_CLOTHES_H
#define GUARD_CLOTHES_H

#include "constants/clothes.h"
#include "sprite.h"

struct ClothesItem
{
    u8 name[24];
    u8 category;
    u8 shape;        // variante da folha de sprite (CLOTHES_SHAPE_NONE = nada)
    u16 color;       // cor principal RGB555 (usada conforme a categoria)
    u16 price;       // 0 = ja pertence ao jogador (se desbloqueado)
    u8 shops;        // bitmask de lojas que vendem o item
    u16 unlockFlag;  // flag exigida (0 = sempre disponivel)
};

extern u16 gClothesPalette[16];
extern struct SpriteFrameImage gClothesPicTable[CLOTHES_NUM_FRAMES];
extern u8 *gClothesFrameBuf;

u16 Clothes_NumItems(void);
const struct ClothesItem *Clothes_GetItem(u8 id);
void Clothes_InitIfNeeded(void);
bool8 Clothes_IsUnlocked(u8 id);
bool8 Clothes_IsOwned(u8 id);
void Clothes_SetOwned(u8 id);
void Clothes_GetEquipped(u8 *equip);
void Clothes_SetEquipped(const u8 *equip);
bool8 Clothes_ComposeOutfit(const u8 *equip);
bool8 Clothes_Prepare(void);
void Clothes_MarkDirty(void);
void Clothes_ResetForNewGame(void);
void Clothes_PrepareBack(void);
void Clothes_LoadBackPalette(u16 dest);
extern struct SpriteFrameImage gClothesBackPicTable[5];
void Clothes_OnHeapReset(void);
void Clothes_RefreshPalettes(void);
u8 Clothes_FilterPlayerGfxId(u8 gfxId);

void CB2_OpenCloset(void);
void Special_OpenClothesShop(void);

#endif // GUARD_CLOTHES_H
