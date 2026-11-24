#include "global.h"
#include "gflib.h"
#include "clothes.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "money.h"
#include "menu.h"
#include "script.h"
#include "constants/event_objects.h"
#include "overworld.h"
#include "strings.h"
#include "sound.h"
#include "task.h"
#include "constants/songs.h"

// ---------------------------------------------------------------------------
// Armario / Provador / Loja de roupas.
// Cima/Baixo: categoria | Esq/Dir: peca (previa em tempo real) | A: comprar (loja)
// START: confirmar | B: cancelar.  Previa em 3 angulos: frente, costas, lado.
// ---------------------------------------------------------------------------

enum { ST_FADEIN, ST_INPUT, ST_BUY_ASK, ST_MESSAGE, ST_FADEOUT };

struct ClothesMenu
{
    u8 shop;
    u8 cat;
    u8 catTop;
    u8 listLen;
    u8 listPos;
    u8 state;
    u8 confirmed;
    u8 spriteIds[3];
    u8 trial[CLOTHES_CAT_COUNT];
    u8 list[CLOTHES_MAX_ITEMS];
    const u8 *msg;
    MainCallback returnCb;
};

static EWRAM_DATA struct ClothesMenu *sMenu = NULL;

static const u8 sText_Closet[]      = _("ARMÁRIO");
static const u8 sText_ShopPallet[]  = _("LOJA ESTILO");
static const u8 sText_ShopCeladon[] = _("BOUTIQUE CELADON");
static const u8 sText_Money[]       = _("Dinheiro: ¥");
static const u8 sText_HelpCloset[]  = _("Esq/Dir: peça  START Confirmar  B Cancelar");
static const u8 sText_HelpShop[]    = _("Esq/Dir: peça  A Comprar  START Ok  B Sair");
static const u8 sText_Equipped[]    = _("EQUIPADO");
static const u8 sText_Owned[]       = _("ADQUIRIDO");
static const u8 sText_Locked[]      = _("BLOQUEADO - complete uma missao");
static const u8 sText_NotSold[]     = _("Indisponível");
static const u8 sText_Price[]       = _("Preço: ¥");
static const u8 sText_AskBuy[]      = _("Comprar? A: sim  B: nao");
static const u8 sText_Bought[]      = _("Compra realizada!");
static const u8 sText_NoMoney[]     = _("Dinheiro insuficiente!");
static const u8 sText_CantBuy[]     = _("Nao pode comprar este item.");
static const u8 sText_Slash[]       = _("/");

static const u8 sCategoryNames[CLOTHES_CAT_COUNT][16] = {
    [CLOTHES_CAT_BODY]      = _("CORPO"),
    [CLOTHES_CAT_SKIN]      = _("PELE"),
    [CLOTHES_CAT_HAIR]      = _("CABELO"),
    [CLOTHES_CAT_HAIRCOLOR] = _("COR DO CABELO"),
    [CLOTHES_CAT_SHIRT]     = _("CAMISETA"),
    [CLOTHES_CAT_JACKET]    = _("JAQUETA"),
    [CLOTHES_CAT_PANTS]     = _("CALÇA / SHORT"),
    [CLOTHES_CAT_SHOES]     = _("TÊNIS / BOTA"),
    [CLOTHES_CAT_CAP]       = _("BONÉ"),
    [CLOTHES_CAT_HAT]       = _("CHAPÉU"),
    [CLOTHES_CAT_GLASSES]   = _("ÓCULOS"),
    [CLOTHES_CAT_PACK]      = _("MOCHILA"),
    [CLOTHES_CAT_ACC]       = _("ACESSÓRIO"),
};

enum { WIN_TOP, WIN_CATS, WIN_ITEM, WIN_HELP };

static const struct WindowTemplate sWindowTemplates[] = {
    [WIN_TOP]  = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 0,  .width = 30, .height = 2,  .paletteNum = 15, .baseBlock = 1},
    [WIN_CATS] = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 2,  .width = 12, .height = 16, .paletteNum = 15, .baseBlock = 61},
    [WIN_ITEM] = {.bg = 0, .tilemapLeft = 12, .tilemapTop = 2,  .width = 18, .height = 8,  .paletteNum = 15, .baseBlock = 253},
    [WIN_HELP] = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 18, .width = 30, .height = 2,  .paletteNum = 15, .baseBlock = 397},
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
};

static const u16 sMenuPalette[16] = {
    RGB(26, 28, 31), RGB(31, 31, 31), RGB(4, 4, 6), RGB(17, 17, 20),
    RGB(26, 4, 4), RGB(31, 20, 20), RGB(3, 18, 5), RGB(20, 28, 20),
    RGB(5, 10, 24), RGB(20, 24, 31), RGB(18, 22, 31), RGB(31, 31, 31),
    RGB(31, 31, 31), RGB(31, 31, 31), RGB(31, 31, 31), RGB(31, 31, 31),
};

static const u8 sColorDark[]  = {TEXT_COLOR_TRANSPARENT, 2, 3};
static const u8 sColorRed[]   = {TEXT_COLOR_TRANSPARENT, 4, 5};
static const u8 sColorGreen[] = {TEXT_COLOR_TRANSPARENT, 6, 7};
static const u8 sColorBlue[]  = {TEXT_COLOR_TRANSPARENT, 8, 3};

static void Task_ClothesMenu(u8 taskId);

static const u8 *ShopName(void)
{
    if (sMenu->shop == CLOTHES_SHOP_PALLET)
        return sText_ShopPallet;
    if (sMenu->shop == CLOTHES_SHOP_CELADON)
        return sText_ShopCeladon;
    return sText_Closet;
}

static bool8 InShop(void)
{
    return sMenu->shop != CLOTHES_SHOP_CLOSET;
}

static void BuildList(void)
{
    u16 i;
    u8 saved[CLOTHES_CAT_COUNT];
    const struct ClothesItem *item;

    sMenu->listLen = 0;
    sMenu->listPos = 0;
    for (i = 0; i < Clothes_NumItems(); i++)
    {
        item = Clothes_GetItem(i);
        if (item->category != sMenu->cat)
            continue;
        if (Clothes_IsOwned(i) || (InShop() && (item->shops & (1 << sMenu->shop))))
        {
            if (sMenu->trial[sMenu->cat] == i)
                sMenu->listPos = sMenu->listLen;
            sMenu->list[sMenu->listLen++] = i;
        }
    }
    if (sMenu->listLen == 0)
    {
        Clothes_GetEquipped(saved);
        sMenu->list[0] = saved[sMenu->cat];
        sMenu->listLen = 1;
    }
}

static void RefreshPreview(void)
{
    u8 i;

    Clothes_ComposeOutfit(sMenu->trial);
    Clothes_RefreshPalettes();
    for (i = 0; i < 3; i++)
    {
        if (sMenu->spriteIds[i] != MAX_SPRITES)
            StartSpriteAnim(&gSprites[sMenu->spriteIds[i]], i);
    }
}

static void Print(u8 win, const u8 *str, u8 x, u8 y, u8 font, const u8 *colors)
{
    AddTextPrinterParameterized4(win, font, x, y, 0, 0, colors, 0, str);
}

static void Flush(u8 win)
{
    PutWindowTilemap(win);
    CopyWindowToVram(win, COPYWIN_FULL);
}

static void DrawTop(void)
{
    FillWindowPixelBuffer(WIN_TOP, PIXEL_FILL(0));
    Print(WIN_TOP, ShopName(), 4, 0, FONT_NORMAL, sColorBlue);
    if (InShop())
    {
        ConvertIntToDecimalStringN(gStringVar1, GetMoney(&gSaveBlock1Ptr->money), STR_CONV_MODE_LEFT_ALIGN, 7);
        StringCopy(gStringVar4, sText_Money);
        StringAppend(gStringVar4, gStringVar1);
        Print(WIN_TOP, gStringVar4, 120, 2, FONT_SMALL, sColorDark);
    }
    Flush(WIN_TOP);
}

static void DrawHelp(void)
{
    FillWindowPixelBuffer(WIN_HELP, PIXEL_FILL(0));
    Print(WIN_HELP, InShop() ? sText_HelpShop : sText_HelpCloset, 4, 0, FONT_SMALL, sColorDark);
    Flush(WIN_HELP);
}

static void DrawCats(void)
{
    u8 i, cat, y;

    FillWindowPixelBuffer(WIN_CATS, PIXEL_FILL(0));
    if (sMenu->cat < sMenu->catTop)
        sMenu->catTop = sMenu->cat;
    if (sMenu->cat >= sMenu->catTop + 8)
        sMenu->catTop = sMenu->cat - 7;
    for (i = 0; i < 8; i++)
    {
        cat = sMenu->catTop + i;
        if (cat >= CLOTHES_CAT_COUNT)
            break;
        y = i * 16;
        if (cat == sMenu->cat)
            FillWindowPixelRect(WIN_CATS, PIXEL_FILL(10), 0, y, 92, 16);
        Print(WIN_CATS, sCategoryNames[cat], 4, y, FONT_NORMAL, cat == sMenu->cat ? sColorRed : sColorDark);
    }
    Flush(WIN_CATS);
}

static void DrawItem(void)
{
    u8 id;
    u8 saved[CLOTHES_CAT_COUNT];
    const struct ClothesItem *item;

    Clothes_GetEquipped(saved);
    FillWindowPixelBuffer(WIN_ITEM, PIXEL_FILL(0));
    id = sMenu->list[sMenu->listPos];
    item = Clothes_GetItem(id);
    Print(WIN_ITEM, item->name, 4, 0, FONT_NORMAL, sColorDark);
    ConvertIntToDecimalStringN(gStringVar1, sMenu->listPos + 1, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, sMenu->listLen, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringCopy(gStringVar4, gStringVar1);
    StringAppend(gStringVar4, sText_Slash);
    StringAppend(gStringVar4, gStringVar2);
    Print(WIN_ITEM, gStringVar4, 4, 16, FONT_SMALL, sColorDark);
    if (!Clothes_IsUnlocked(id))
        Print(WIN_ITEM, sText_Locked, 4, 30, FONT_SMALL, sColorRed);
    else if (saved[sMenu->cat] == id)
        Print(WIN_ITEM, sText_Equipped, 4, 30, FONT_SMALL, sColorGreen);
    else if (Clothes_IsOwned(id))
        Print(WIN_ITEM, sText_Owned, 4, 30, FONT_SMALL, sColorGreen);
    else if (InShop() && (item->shops & (1 << sMenu->shop)))
    {
        ConvertIntToDecimalStringN(gStringVar1, item->price, STR_CONV_MODE_LEFT_ALIGN, 5);
        StringCopy(gStringVar4, sText_Price);
        StringAppend(gStringVar4, gStringVar1);
        Print(WIN_ITEM, gStringVar4, 4, 30, FONT_SMALL, sColorBlue);
    }
    else
        Print(WIN_ITEM, sText_NotSold, 4, 30, FONT_SMALL, sColorRed);
    if (sMenu->msg != NULL)
        Print(WIN_ITEM, sMenu->msg, 4, 46, FONT_SMALL, sColorRed);
    Flush(WIN_ITEM);
}

static void Redraw(void)
{
    DrawItem();
}

static void SelectItem(s8 delta)
{
    u8 id;
    u8 other;

    sMenu->listPos = (sMenu->listPos + sMenu->listLen + delta) % sMenu->listLen;
    id = sMenu->list[sMenu->listPos];
    sMenu->trial[sMenu->cat] = id;
    // bone e chapeu sao exclusivos
    if (Clothes_GetItem(id)->shape != CLOTHES_SHAPE_NONE)
    {
        if (sMenu->cat == CLOTHES_CAT_CAP)
            sMenu->trial[CLOTHES_CAT_HAT] = Clothes_GetItem(id) - Clothes_GetItem(0) >= 0 ? 0 : 0;
    }
    if (sMenu->cat == CLOTHES_CAT_CAP || sMenu->cat == CLOTHES_CAT_HAT)
    {
        if (Clothes_GetItem(id)->shape != CLOTHES_SHAPE_NONE)
        {
            other = (sMenu->cat == CLOTHES_CAT_CAP) ? CLOTHES_CAT_HAT : CLOTHES_CAT_CAP;
            for (id = 0; id < Clothes_NumItems(); id++)
            {
                if (Clothes_GetItem(id)->category == other && Clothes_GetItem(id)->shape == CLOTHES_SHAPE_NONE)
                {
                    sMenu->trial[other] = id;
                    break;
                }
            }
        }
    }
    sMenu->msg = NULL;
    RefreshPreview();
    Redraw();
}

static void ChangeCategory(s8 delta)
{
    sMenu->cat = (sMenu->cat + CLOTHES_CAT_COUNT + delta) % CLOTHES_CAT_COUNT;
    sMenu->msg = NULL;
    BuildList();
    DrawCats();
    DrawItem();
}

static void TryBuy(void)
{
    u8 id = sMenu->list[sMenu->listPos];
    const struct ClothesItem *item = Clothes_GetItem(id);

    if (Clothes_IsOwned(id) || !Clothes_IsUnlocked(id) || !(item->shops & (1 << sMenu->shop)))
    {
        PlaySE(SE_BOO);
        sMenu->msg = sText_CantBuy;
        Redraw();
        return;
    }
    sMenu->msg = sText_AskBuy;
    sMenu->state = ST_BUY_ASK;
    Redraw();
}

static void FinishAndExit(bool8 confirm)
{
    u8 equip[CLOTHES_CAT_COUNT];
    u8 i;

    sMenu->confirmed = confirm;
    if (confirm)
    {
        Clothes_GetEquipped(equip);
        for (i = 0; i < CLOTHES_CAT_COUNT; i++)
        {
            if (Clothes_IsOwned(sMenu->trial[i]))
                equip[i] = sMenu->trial[i];
        }
        Clothes_SetEquipped(equip);
    }
    Clothes_MarkDirty();
    Clothes_Prepare();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    sMenu->state = ST_FADEOUT;
}

static void Task_ClothesMenu(u8 taskId)
{
    u8 id;
    const struct ClothesItem *item;

    switch (sMenu->state)
    {
    case ST_FADEIN:
        if (!gPaletteFade.active)
            sMenu->state = ST_INPUT;
        break;
    case ST_INPUT:
        if (JOY_NEW(DPAD_UP))
            PlaySE(SE_SELECT), ChangeCategory(-1);
        else if (JOY_NEW(DPAD_DOWN))
            PlaySE(SE_SELECT), ChangeCategory(1);
        else if (JOY_NEW(DPAD_LEFT))
            PlaySE(SE_SELECT), SelectItem(-1);
        else if (JOY_NEW(DPAD_RIGHT))
            PlaySE(SE_SELECT), SelectItem(1);
        else if (JOY_NEW(A_BUTTON))
        {
            if (InShop())
                TryBuy();
            else
                FinishAndExit(TRUE);
        }
        else if (JOY_NEW(START_BUTTON))
            FinishAndExit(TRUE);
        else if (JOY_NEW(B_BUTTON))
            FinishAndExit(FALSE);
        break;
    case ST_BUY_ASK:
        if (JOY_NEW(A_BUTTON))
        {
            id = sMenu->list[sMenu->listPos];
            item = Clothes_GetItem(id);
            if (IsEnoughMoney(&gSaveBlock1Ptr->money, item->price))
            {
                RemoveMoney(&gSaveBlock1Ptr->money, item->price);
                Clothes_SetOwned(id);
                PlaySE(SE_SHOP);
                sMenu->msg = sText_Bought;
                DrawTop();
            }
            else
            {
                PlaySE(SE_BOO);
                sMenu->msg = sText_NoMoney;
            }
            sMenu->state = ST_MESSAGE;
            Redraw();
        }
        else if (JOY_NEW(B_BUTTON))
        {
            sMenu->msg = NULL;
            sMenu->state = ST_INPUT;
            Redraw();
        }
        break;
    case ST_MESSAGE:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            sMenu->msg = NULL;
            sMenu->state = ST_INPUT;
            Redraw();
        }
        break;
    case ST_FADEOUT:
        if (!gPaletteFade.active)
        {
            MainCallback cb = sMenu->returnCb;

            DestroyTask(taskId);
            FreeAllWindowBuffers();
            Free(sMenu);
            sMenu = NULL;
            SetMainCallback2(cb);
        }
        break;
    }
}

static void CB2_ClothesMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_ClothesMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_InitClothesMenu(void)
{
    u8 i;
    static const s16 sPreviewX[3] = {132, 170, 208};

    SetVBlankCallback(NULL);
    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetTasks();
    LoadPalette(sMenuPalette, BG_PLTT_ID(15), sizeof(sMenuPalette));
    LoadPalette(sMenuPalette, BG_PLTT_ID(0), 2);

    Clothes_GetEquipped(sMenu->trial);
    Clothes_ComposeOutfit(sMenu->trial);
    for (i = 0; i < 3; i++)
    {
        sMenu->spriteIds[i] = CreateObjectGraphicsSprite(OBJ_EVENT_GFX_PLAYER_CUSTOM, SpriteCallbackDummy, sPreviewX[i], 118, 0);
        if (sMenu->spriteIds[i] != MAX_SPRITES)
            StartSpriteAnim(&gSprites[sMenu->spriteIds[i]], i);
    }
    Clothes_RefreshPalettes();
    BuildList();
    DrawTop();
    DrawCats();
    DrawItem();
    DrawHelp();
    ShowBg(0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_BG0_ON);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    CreateTask(Task_ClothesMenu, 0);
    SetVBlankCallback(VBlankCB_ClothesMenu);
    SetMainCallback2(CB2_ClothesMenu);
}

static void OpenMenu(u8 shop, MainCallback returnCb)
{
    Clothes_InitIfNeeded();
    sMenu = AllocZeroed(sizeof(struct ClothesMenu));
    sMenu->shop = shop;
    sMenu->returnCb = returnCb;
    sMenu->state = ST_FADEIN;
    SetMainCallback2(CB2_InitClothesMenu);
}

void CB2_OpenCloset(void)
{
    OpenMenu(CLOTHES_SHOP_CLOSET, gMain.savedCallback != NULL ? gMain.savedCallback : CB2_ReturnToFieldWithOpenMenu);
}

static void CB2_OpenShop(void)
{
    OpenMenu(gSpecialVar_0x8004, CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// special: abre a loja de roupas cujo id esta em VAR_0x8004
void Special_OpenClothesShop(void)
{
    SetMainCallback2(CB2_OpenShop);
    LockPlayerFieldControls();
}
