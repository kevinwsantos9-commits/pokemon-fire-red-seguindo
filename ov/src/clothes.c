#include "global.h"
#include "gflib.h"
#include "malloc.h"
#include "clothes.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "event_data.h"

// ---------------------------------------------------------------------------
// Sistema de roupas em camadas.
//
// O treinador e montado em tempo de execucao: cada peca equipada e uma folha de
// 20 quadros (16x32, 4bpp) em graphics/clothes/. As camadas sao empilhadas numa
// ordem fixa, o contorno e gerado automaticamente em volta da silhueta final e as
// cores vem de uma paleta de 16 entradas montada a partir das pecas equipadas.
// O resultado vai para um buffer no heap (5 KB) usado como tabela de quadros do
// sprite do jogador (OBJ_EVENT_GFX_PLAYER_CUSTOM). Nada disso usa EWRAM fixa alem
// de ~200 bytes, pois a EWRAM do FireRed esta 99,6% ocupada.
// ---------------------------------------------------------------------------

#define NUM_TABLE_ITEMS (sizeof(sItems) / sizeof(sItems[0]))

static const struct ClothesItem sItems[] = {
#include "data/clothes_items.h"
};

EWRAM_DATA u16 gClothesPalette[16] = {0};
EWRAM_DATA struct SpriteFrameImage gClothesPicTable[CLOTHES_NUM_FRAMES] = {0};
EWRAM_DATA u8 *gClothesFrameBuf = NULL;
static EWRAM_DATA bool8 sClothesDirty = TRUE;

// Folhas de sprite (geradas por tools/clothes_gen/gen_layers.py)
static const u8 sBodyM[]    = INCBIN_U8("graphics/clothes/body_m.4bpp");
static const u8 sBodyF[]    = INCBIN_U8("graphics/clothes/body_f.4bpp");
static const u8 sHair0[]    = INCBIN_U8("graphics/clothes/hair_0.4bpp");
static const u8 sHair1[]    = INCBIN_U8("graphics/clothes/hair_1.4bpp");
static const u8 sHair2[]    = INCBIN_U8("graphics/clothes/hair_2.4bpp");
static const u8 sHair3[]    = INCBIN_U8("graphics/clothes/hair_3.4bpp");
static const u8 sHair4[]    = INCBIN_U8("graphics/clothes/hair_4.4bpp");
static const u8 sShirt0[]   = INCBIN_U8("graphics/clothes/shirt_0.4bpp");
static const u8 sShirt1[]   = INCBIN_U8("graphics/clothes/shirt_1.4bpp");
static const u8 sJacket0[]  = INCBIN_U8("graphics/clothes/jacket_0.4bpp");
static const u8 sJacket1[]  = INCBIN_U8("graphics/clothes/jacket_1.4bpp");
static const u8 sPants0[]   = INCBIN_U8("graphics/clothes/pants_0.4bpp");
static const u8 sPants1[]   = INCBIN_U8("graphics/clothes/pants_1.4bpp");
static const u8 sShoes0[]   = INCBIN_U8("graphics/clothes/shoes_0.4bpp");
static const u8 sShoes1[]   = INCBIN_U8("graphics/clothes/shoes_1.4bpp");
static const u8 sHat0[]     = INCBIN_U8("graphics/clothes/hat_0.4bpp");
static const u8 sHat1[]     = INCBIN_U8("graphics/clothes/hat_1.4bpp");
static const u8 sHat2[]     = INCBIN_U8("graphics/clothes/hat_2.4bpp");
static const u8 sGlasses0[] = INCBIN_U8("graphics/clothes/glasses_0.4bpp");
static const u8 sGlasses1[] = INCBIN_U8("graphics/clothes/glasses_1.4bpp");
static const u8 sPack0[]    = INCBIN_U8("graphics/clothes/pack_0.4bpp");
static const u8 sPack1[]    = INCBIN_U8("graphics/clothes/pack_1.4bpp");
static const u8 sAcc0[]     = INCBIN_U8("graphics/clothes/acc_0.4bpp");
static const u8 sAcc1[]     = INCBIN_U8("graphics/clothes/acc_1.4bpp");

static const u8 *const sHairSheets[]    = {sHair0, sHair1, sHair2, sHair3, sHair4};
static const u8 *const sShirtSheets[]   = {sShirt0, sShirt1};
static const u8 *const sJacketSheets[]  = {sJacket0, sJacket1};
static const u8 *const sPantsSheets[]   = {sPants0, sPants1};
static const u8 *const sShoesSheets[]   = {sShoes0, sShoes1};
static const u8 *const sHatSheets[]     = {sHat0, sHat1, sHat2};
static const u8 *const sGlassesSheets[] = {sGlasses0, sGlasses1};
static const u8 *const sPackSheets[]    = {sPack0, sPack1};
static const u8 *const sAccSheets[]     = {sAcc0, sAcc1};

// Quadros virados para o norte (a mochila passa para a frente do corpo)
static const u8 sFrameIsNorth[CLOTHES_NUM_FRAMES] = {
    0, 1, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 0
};


// Folhas do sprite de batalha (64x64, 5 quadros) - tools/clothes_gen/gen_back.py
static const u8 sBBody[] = INCBIN_U8("graphics/clothes/back_body.4bpp");
static const u8 sBHair0[] = INCBIN_U8("graphics/clothes/back_hair_0.4bpp");
static const u8 sBHair1[] = INCBIN_U8("graphics/clothes/back_hair_1.4bpp");
static const u8 sBHair2[] = INCBIN_U8("graphics/clothes/back_hair_2.4bpp");
static const u8 sBHair3[] = INCBIN_U8("graphics/clothes/back_hair_3.4bpp");
static const u8 sBHair4[] = INCBIN_U8("graphics/clothes/back_hair_4.4bpp");
static const u8 sBShirt0[] = INCBIN_U8("graphics/clothes/back_shirt_0.4bpp");
static const u8 sBShirt1[] = INCBIN_U8("graphics/clothes/back_shirt_1.4bpp");
static const u8 sBJacket0[] = INCBIN_U8("graphics/clothes/back_jacket_0.4bpp");
static const u8 sBJacket1[] = INCBIN_U8("graphics/clothes/back_jacket_1.4bpp");
static const u8 sBPants0[] = INCBIN_U8("graphics/clothes/back_pants_0.4bpp");
static const u8 sBPants1[] = INCBIN_U8("graphics/clothes/back_pants_1.4bpp");
static const u8 sBShoes0[] = INCBIN_U8("graphics/clothes/back_shoes_0.4bpp");
static const u8 sBShoes1[] = INCBIN_U8("graphics/clothes/back_shoes_1.4bpp");
static const u8 sBHat0[] = INCBIN_U8("graphics/clothes/back_hat_0.4bpp");
static const u8 sBHat1[] = INCBIN_U8("graphics/clothes/back_hat_1.4bpp");
static const u8 sBHat2[] = INCBIN_U8("graphics/clothes/back_hat_2.4bpp");
static const u8 sBPack0[] = INCBIN_U8("graphics/clothes/back_pack_0.4bpp");
static const u8 sBPack1[] = INCBIN_U8("graphics/clothes/back_pack_1.4bpp");
static const u8 sBAcc0[] = INCBIN_U8("graphics/clothes/back_acc_0.4bpp");
static const u8 sBAcc1[] = INCBIN_U8("graphics/clothes/back_acc_1.4bpp");

static const u8 *const sBHairSheets[]    = {sBHair0, sBHair1, sBHair2, sBHair3, sBHair4};
static const u8 *const sBShirtSheets[]   = {sBShirt0, sBShirt1};
static const u8 *const sBJacketSheets[]  = {sBJacket0, sBJacket1};
static const u8 *const sBPantsSheets[]   = {sBPants0, sBPants1};
static const u8 *const sBShoesSheets[]   = {sBShoes0, sBShoes1};
static const u8 *const sBHatSheets[]     = {sBHat0, sBHat1, sBHat2};
static const u8 *const sBPackSheets[]    = {sBPack0, sBPack1};
static const u8 *const sBAccSheets[]     = {sBAcc0, sBAcc1};

EWRAM_DATA u16 gClothesBackPalette[16] = {0};
EWRAM_DATA struct SpriteFrameImage gClothesBackPicTable[5] = {0};
static EWRAM_DATA u8 *sBackBuf = NULL;
static EWRAM_DATA bool8 sBackDirty = TRUE;

STATIC_ASSERT(sizeof(struct ClothesSave) == 0x20, ClothesSaveSize);

u16 Clothes_NumItems(void)
{
    return NUM_TABLE_ITEMS;
}

const struct ClothesItem *Clothes_GetItem(u8 id)
{
    if (id >= NUM_TABLE_ITEMS)
        id = 0;
    return &sItems[id];
}

static u8 FirstItemOfCategory(u8 cat)
{
    u8 i;

    for (i = 0; i < NUM_TABLE_ITEMS; i++)
    {
        if (sItems[i].category == cat)
            return i;
    }
    return 0;
}

bool8 Clothes_IsUnlocked(u8 id)
{
    const struct ClothesItem *item = Clothes_GetItem(id);

    if (item->unlockFlag == 0)
        return TRUE;
    return FlagGet(item->unlockFlag);
}

bool8 Clothes_IsOwned(u8 id)
{
    const struct ClothesItem *item = Clothes_GetItem(id);

    if (!Clothes_IsUnlocked(id))
        return FALSE;
    if (item->price == 0)
        return TRUE;
    return (gSaveBlock2Ptr->clothes.owned[id >> 3] >> (id & 7)) & 1;
}

void Clothes_SetOwned(u8 id)
{
    if (id < NUM_TABLE_ITEMS)
        gSaveBlock2Ptr->clothes.owned[id >> 3] |= (1 << (id & 7));
}

void Clothes_ResetForNewGame(void)
{
    memset(&gSaveBlock2Ptr->clothes, 0, sizeof(gSaveBlock2Ptr->clothes));
    sClothesDirty = TRUE;
}

void Clothes_InitIfNeeded(void)
{
    u8 i;
    struct ClothesSave *save = &gSaveBlock2Ptr->clothes;

    if (save->magic == CLOTHES_SAVE_MAGIC)
        return;
    memset(save, 0, sizeof(*save));
    for (i = 0; i < CLOTHES_CAT_COUNT; i++)
        save->equipped[i] = FirstItemOfCategory(i);
    // Visual inicial parecido com o do jogo original: bone vermelho (menino) ou branco (menina)
    save->equipped[CLOTHES_CAT_BODY] = FirstItemOfCategory(CLOTHES_CAT_BODY) + (gSaveBlock2Ptr->playerGender == FEMALE);
    save->equipped[CLOTHES_CAT_CAP] = FirstItemOfCategory(CLOTHES_CAT_CAP) + 1 + (gSaveBlock2Ptr->playerGender == FEMALE);
    save->equipped[CLOTHES_CAT_HAIR] = FirstItemOfCategory(CLOTHES_CAT_HAIR) + (gSaveBlock2Ptr->playerGender == FEMALE ? 3 : 0);
    save->magic = CLOTHES_SAVE_MAGIC;
    sClothesDirty = TRUE;
}

void Clothes_GetEquipped(u8 *equip)
{
    u8 i;

    Clothes_InitIfNeeded();
    for (i = 0; i < CLOTHES_CAT_COUNT; i++)
    {
        equip[i] = gSaveBlock2Ptr->clothes.equipped[i];
        if (equip[i] >= NUM_TABLE_ITEMS || sItems[equip[i]].category != i)
            equip[i] = FirstItemOfCategory(i);
    }
}

void Clothes_SetEquipped(const u8 *equip)
{
    u8 i;

    Clothes_InitIfNeeded();
    for (i = 0; i < CLOTHES_CAT_COUNT; i++)
        gSaveBlock2Ptr->clothes.equipped[i] = equip[i];
    // O corpo escolhido define o genero do treinador (sprites de moto/surf, carta de treinador...)
    gSaveBlock2Ptr->playerGender = sItems[equip[CLOTHES_CAT_BODY]].shape;
    gPlayerAvatar.gender = gSaveBlock2Ptr->playerGender;
    sClothesDirty = TRUE;
}

void Clothes_MarkDirty(void)
{
    sClothesDirty = TRUE;
    sBackDirty = TRUE;
}

static u16 Shade(u16 c, u8 pct)
{
    u16 r = (c & 0x1F) * pct / 100;
    u16 g = ((c >> 5) & 0x1F) * pct / 100;
    u16 b = ((c >> 10) & 0x1F) * pct / 100;

    return RGB(r, g, b);
}

static void BuildPalette(const u8 *eq)
{
    u16 *p = gClothesPalette;
    u16 gear;
    u16 head;
    const struct ClothesItem *cap = &sItems[eq[CLOTHES_CAT_CAP]];
    const struct ClothesItem *hat = &sItems[eq[CLOTHES_CAT_HAT]];
    const struct ClothesItem *pack = &sItems[eq[CLOTHES_CAT_PACK]];
    const struct ClothesItem *acc = &sItems[eq[CLOTHES_CAT_ACC]];

    p[0] = RGB(0, 0, 0);
    p[1] = RGB(3, 2, 5);                                 // contorno
    p[2] = sItems[eq[CLOTHES_CAT_SKIN]].color;           // pele
    p[3] = Shade(p[2], 82);
    p[4] = sItems[eq[CLOTHES_CAT_HAIRCOLOR]].color;      // cabelo
    p[5] = Shade(p[4], 70);
    head = RGB(25, 5, 5);
    if (cap->shape != CLOTHES_SHAPE_NONE)
        head = cap->color;
    else if (hat->shape != CLOTHES_SHAPE_NONE)
        head = hat->color;
    p[6] = head;                                         // bone / chapeu
    p[7] = Shade(head, 70);
    gear = RGB(28, 22, 5);
    if (pack->shape != CLOTHES_SHAPE_NONE)
        gear = pack->color;
    else if (acc->shape != CLOTHES_SHAPE_NONE)
        gear = acc->color;
    p[14] = gear;                                        // mochila / acessorio
    p[9] = sItems[eq[CLOTHES_CAT_SHIRT]].color;          // camiseta
    p[10] = Shade(p[9], 78);
    p[11] = sItems[eq[CLOTHES_CAT_JACKET]].color;        // jaqueta
    p[12] = Shade(p[11], 70);
    p[13] = sItems[eq[CLOTHES_CAT_PANTS]].color;         // calca
    p[8] = Shade(p[13], 72);                             // sombra da calca
    p[15] = sItems[eq[CLOTHES_CAT_SHOES]].color;         // calcado
}

static const u8 *SheetFor(const u8 *eq, u8 cat, const u8 *const *sheets, u8 count)
{
    u8 shape = sItems[eq[cat]].shape;

    if (shape == CLOTHES_SHAPE_NONE || shape >= count)
        return NULL;
    return sheets[shape];
}

// Desenha uma camada (4bpp, ordem de tiles 2x4) num buffer de 1 byte por pixel.
static void BlitLayer(u8 *px, const u8 *sheet, u8 frame)
{
    const u8 *src = sheet + frame * CLOTHES_FRAME_SIZE;
    u8 t, row, b, v;
    u8 *dst;

    for (t = 0; t < 8; t++)
    {
        for (row = 0; row < 8; row++)
        {
            dst = px + (((t >> 1) * 8 + row) * 16) + ((t & 1) * 8);
            for (b = 0; b < 4; b++)
            {
                v = *src++;
                if (v & 0x0F)
                    dst[b * 2] = v & 0x0F;
                if (v >> 4)
                    dst[b * 2 + 1] = v >> 4;
            }
        }
    }
}

// Contorno automatico: todo pixel vazio encostado (4 vizinhos) na figura vira contorno.
static void AddOutline(u8 *px)
{
    s32 x, y;
    u8 *p;

    for (y = 0; y < 32; y++)
    {
        for (x = 0; x < 16; x++)
        {
            p = px + y * 16 + x;
            if (*p != 0)
                continue;
            if ((x > 0 && p[-1] != 0 && p[-1] != 0xFF)
             || (x < 15 && p[1] != 0 && p[1] != 0xFF)
             || (y > 0 && p[-16] != 0 && p[-16] != 0xFF)
             || (y < 31 && p[16] != 0 && p[16] != 0xFF))
                *p = 0xFF;
        }
    }
    for (x = 0; x < 16 * 32; x++)
    {
        if (px[x] == 0xFF)
            px[x] = 1;
    }
}

static void PackFrame(u8 *dst, const u8 *px)
{
    u8 t, row, b;
    const u8 *src;

    for (t = 0; t < 8; t++)
    {
        for (row = 0; row < 8; row++)
        {
            src = px + (((t >> 1) * 8 + row) * 16) + ((t & 1) * 8);
            for (b = 0; b < 4; b++)
                *dst++ = (src[b * 2] & 0x0F) | (src[b * 2 + 1] << 4);
        }
    }
}

static void ComposeFrame(u8 *dst, const u8 *eq, u8 frame)
{
    u8 px[16 * 32];
    const u8 *layers[12];
    u8 n = 0, i;
    const u8 *pack = SheetFor(eq, CLOTHES_CAT_PACK, sPackSheets, 2);

    memset(px, 0, sizeof(px));
    if (pack != NULL && !sFrameIsNorth[frame])
        layers[n++] = pack;
    layers[n++] = (sItems[eq[CLOTHES_CAT_BODY]].shape == FEMALE) ? sBodyF : sBodyM;
    layers[n] = SheetFor(eq, CLOTHES_CAT_PANTS, sPantsSheets, 2);   if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_SHOES, sShoesSheets, 2);   if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_SHIRT, sShirtSheets, 2);   if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_JACKET, sJacketSheets, 2); if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_HAIR, sHairSheets, 5);     if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_GLASSES, sGlassesSheets, 2); if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_ACC, sAccSheets, 2);       if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_CAP, sHatSheets, 3);       if (layers[n] != NULL) n++;
    layers[n] = SheetFor(eq, CLOTHES_CAT_HAT, sHatSheets, 3);       if (layers[n] != NULL) n++;
    if (pack != NULL && sFrameIsNorth[frame])
        layers[n++] = pack;

    for (i = 0; i < n; i++)
        BlitLayer(px, layers[i], frame);
    AddOutline(px);
    PackFrame(dst, px);
}

bool8 Clothes_ComposeOutfit(const u8 *equip)
{
    u8 f;

    if (gClothesFrameBuf == NULL)
    {
        gClothesFrameBuf = Alloc(CLOTHES_NUM_FRAMES * CLOTHES_FRAME_SIZE);
        if (gClothesFrameBuf == NULL)
            return FALSE;
    }
    BuildPalette(equip);
    for (f = 0; f < CLOTHES_NUM_FRAMES; f++)
    {
        ComposeFrame(gClothesFrameBuf + f * CLOTHES_FRAME_SIZE, equip, f);
        gClothesPicTable[f].data = gClothesFrameBuf + f * CLOTHES_FRAME_SIZE;
        gClothesPicTable[f].size = CLOTHES_FRAME_SIZE;
    }
    return TRUE;
}

// Garante que buffer, tabela de quadros e paleta correspondem ao visual salvo.
bool8 Clothes_Prepare(void)
{
    u8 equip[CLOTHES_CAT_COUNT];

    Clothes_InitIfNeeded();
    if (gClothesFrameBuf != NULL && !sClothesDirty)
        return TRUE;
    Clothes_GetEquipped(equip);
    if (!Clothes_ComposeOutfit(equip))
        return FALSE;
    sClothesDirty = FALSE;
    return TRUE;
}


// ---------------------------------------------------------------------------
// Sprite de batalha (costas, 64x64, 5 quadros: parado + arremesso)
// ---------------------------------------------------------------------------
#define BACK_FRAME_SIZE 2048

static void BuildBackPalette(const u8 *eq)
{
    u16 *p = gClothesBackPalette;
    u16 head = RGB(25, 5, 5);
    u16 gear = RGB(28, 22, 5);
    const struct ClothesItem *cap = &sItems[eq[CLOTHES_CAT_CAP]];
    const struct ClothesItem *hat = &sItems[eq[CLOTHES_CAT_HAT]];
    const struct ClothesItem *pack = &sItems[eq[CLOTHES_CAT_PACK]];
    const struct ClothesItem *acc = &sItems[eq[CLOTHES_CAT_ACC]];

    if (cap->shape != CLOTHES_SHAPE_NONE)
        head = cap->color;
    else if (hat->shape != CLOTHES_SHAPE_NONE)
        head = hat->color;
    if (pack->shape != CLOTHES_SHAPE_NONE)
        gear = pack->color;
    else if (acc->shape != CLOTHES_SHAPE_NONE)
        gear = acc->color;
    p[0] = RGB(0, 0, 0);
    p[1] = RGB(3, 2, 5);
    p[2] = sItems[eq[CLOTHES_CAT_SKIN]].color;
    p[3] = Shade(p[2], 82);
    p[4] = sItems[eq[CLOTHES_CAT_HAIRCOLOR]].color;
    p[5] = Shade(p[4], 65);
    p[6] = head;
    p[7] = Shade(head, 70);
    p[8] = sItems[eq[CLOTHES_CAT_SHIRT]].color;
    p[9] = Shade(p[8], 78);
    p[10] = sItems[eq[CLOTHES_CAT_JACKET]].color;
    p[11] = Shade(p[10], 70);
    p[12] = sItems[eq[CLOTHES_CAT_PANTS]].color;
    p[13] = Shade(p[12], 72);
    p[14] = gear;
    p[15] = sItems[eq[CLOTHES_CAT_SHOES]].color;
}

static void BlitBackLayer(u8 *px, const u8 *sheet, u8 frame)
{
    const u8 *src = sheet + frame * BACK_FRAME_SIZE;
    u8 t, row, b, v;
    u8 *dst;

    for (t = 0; t < 64; t++)
    {
        for (row = 0; row < 8; row++)
        {
            dst = px + (((t >> 3) * 8 + row) * 64) + ((t & 7) * 8);
            for (b = 0; b < 4; b++)
            {
                v = *src++;
                if (v & 0x0F)
                    dst[b * 2] = v & 0x0F;
                if (v >> 4)
                    dst[b * 2 + 1] = v >> 4;
            }
        }
    }
}

static void OutlineAndPackBack(u8 *dst, u8 *px)
{
    s32 x, y;
    u8 *p;
    u8 t, row, b;
    const u8 *src;

    for (y = 0; y < 64; y++)
    {
        for (x = 0; x < 64; x++)
        {
            p = px + y * 64 + x;
            if (*p != 0)
                continue;
            if ((x > 0 && p[-1] != 0 && p[-1] != 0xFF)
             || (x < 63 && p[1] != 0 && p[1] != 0xFF)
             || (y > 0 && p[-64] != 0 && p[-64] != 0xFF)
             || (y < 63 && p[64] != 0 && p[64] != 0xFF))
                *p = 0xFF;
        }
    }
    for (x = 0; x < 64 * 64; x++)
    {
        if (px[x] == 0xFF)
            px[x] = 1;
    }
    for (t = 0; t < 64; t++)
    {
        for (row = 0; row < 8; row++)
        {
            src = px + (((t >> 3) * 8 + row) * 64) + ((t & 7) * 8);
            for (b = 0; b < 4; b++)
                *dst++ = (src[b * 2] & 0x0F) | (src[b * 2 + 1] << 4);
        }
    }
}

static bool8 ComposeBack(const u8 *eq)
{
    u8 *px;
    u8 f, n, i;
    const u8 *layers[10];
    static const u8 *const *const tables[] = {
        sBPantsSheets, sBShoesSheets, sBShirtSheets, sBJacketSheets, sBHairSheets, sBAccSheets, sBHatSheets, sBHatSheets, sBPackSheets
    };
    static const u8 cats[] = {
        CLOTHES_CAT_PANTS, CLOTHES_CAT_SHOES, CLOTHES_CAT_SHIRT, CLOTHES_CAT_JACKET, CLOTHES_CAT_HAIR,
        CLOTHES_CAT_ACC, CLOTHES_CAT_CAP, CLOTHES_CAT_HAT, CLOTHES_CAT_PACK
    };
    static const u8 counts[] = {2, 2, 2, 2, 5, 2, 3, 3, 2};

    if (sBackBuf == NULL)
    {
        sBackBuf = Alloc(5 * BACK_FRAME_SIZE);
        if (sBackBuf == NULL)
            return FALSE;
    }
    px = Alloc(64 * 64);
    if (px == NULL)
        return FALSE;
    BuildBackPalette(eq);
    for (f = 0; f < 5; f++)
    {
        memset(px, 0, 64 * 64);
        n = 0;
        layers[n++] = sBBody;
        for (i = 0; i < 9; i++)
        {
            layers[n] = SheetFor(eq, cats[i], tables[i], counts[i]);
            if (layers[n] != NULL)
                n++;
        }
        for (i = 0; i < n; i++)
            BlitBackLayer(px, layers[i], f);
        OutlineAndPackBack(sBackBuf + f * BACK_FRAME_SIZE, px);
        gClothesBackPicTable[f].data = sBackBuf + f * BACK_FRAME_SIZE;
        gClothesBackPicTable[f].size = BACK_FRAME_SIZE;
    }
    Free(px);
    return TRUE;
}

void Clothes_PrepareBack(void)
{
    u8 equip[CLOTHES_CAT_COUNT];

    if (sBackBuf != NULL && !sBackDirty)
        return;
    Clothes_GetEquipped(equip);
    if (ComposeBack(equip))
        sBackDirty = FALSE;
}

void Clothes_LoadBackPalette(u16 dest)
{
    Clothes_PrepareBack();
    LoadPalette(gClothesBackPalette, dest, PLTT_SIZE_4BPP);
}

// Chamado por InitHeap: o buffer deixa de existir quando o heap e reiniciado.
void Clothes_OnHeapReset(void)
{
    gClothesFrameBuf = NULL;
    sClothesDirty = TRUE;
    sBackBuf = NULL;
    sBackDirty = TRUE;
}

void Clothes_RefreshPalettes(void)
{
    u8 idx = IndexOfSpritePaletteTag(CLOTHES_PAL_TAG);

    if (idx != 0xFF)
        LoadPalette(gClothesPalette, OBJ_PLTT_ID(idx), PLTT_SIZE_4BPP);
}

// Troca os sprites normais (Red/Green) pelo sprite personalizado. Outros estados
// (bike, surf, pescar...) continuam usando os sprites originais.
u8 Clothes_FilterPlayerGfxId(u8 gfxId)
{
    if (gfxId == OBJ_EVENT_GFX_RED_NORMAL || gfxId == OBJ_EVENT_GFX_GREEN_NORMAL)
    {
        if (Clothes_Prepare())
            return OBJ_EVENT_GFX_PLAYER_CUSTOM;
    }
    return gfxId;
}
