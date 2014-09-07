#include "skse/SafeWrite.h"
#include "skse/Utilities.h"
#include "skse/GameTypes.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"


namespace GamePatches
{

class WeaponEnchantCraftHook
{
public:
	EnchantmentItem* CraftEvent_Hook(tArray<MagicItem::EffectItem>* effectArray)
	{
		EnchantmentItem* result = CALL_MEMBER_FN(this, CreateOffensiveEnchantment)(effectArray);

		//_MESSAGE("\n\n\n\n\n\n\nHooker called! looks like we've got about... %u effects here... :D\n", effectArray->count);

		return result;
	}

	MEMBER_FN_PREFIX(WeaponEnchantCraftHook);
	DEFINE_MEMBER_FN(CreateOffensiveEnchantment, EnchantmentItem *, 0x00689D30, tArray<MagicItem::EffectItem> * effectArray);

	static void CraftHook_Commit(void)
	{
		WriteRelCall(0x00532FBC, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
		WriteRelCall(0x00851BF0 + 0x23C, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
		WriteRelCall(0x00852DF6, GetFnAddr(&WeaponEnchantCraftHook::CraftEvent_Hook));
	}

};




EnchantmentItem* __stdcall HandleCraftedEnchantment(tArray<MagicItem::EffectItem>* effectArray, EnchantmentItem* createdEnchantment)
{

	_MESSAGE("CreateEnchantment called for enchantment 0x%08X", createdEnchantment->formID);
	for (UInt32 n = 0; n < effectArray->count; n++)
	{
		MagicItem::EffectItem* effectItem = NULL;
		effectItem = &effectArray->arr.entries[n];
		//effectArray->GetNthItem(n, effectItem);
		_MESSAGE("    effect %u  mgef  -  %08X", n, effectItem->mgef->formID);
		_MESSAGE("          magnitude  -  %g", effectItem->magnitude);
		_MESSAGE("           duration  -  %u", effectItem->duration);
		_MESSAGE("               area  -  %u", effectItem->area);
		_MESSAGE("               cost  -  %g", effectItem->cost);
		_MESSAGE("          Condition  -  %08X", effectItem->unk14);
		createdEnchantment->effectItemList.GetNthItem(n, effectItem);
		_MESSAGE("          [area in enchantment == %u]", effectItem->area);
	}
	
	//_MESSAGE("Woo! Hooker visits me with %u effects! enchantment is 0x%08X!", effectArray->count, createdEnchantment->formID);
	return createdEnchantment;
}

static const UInt32 kCreateEnchantment_Hook_Base	= 0x00689D30;
static const UInt32 kCreateEnchantment_Hook_Enter	= kCreateEnchantment_Hook_Base + 0x20;
static const UInt32 kCreateEnchantment_Hook_Return	= kCreateEnchantment_Hook_Base + 0x28;
static const UInt32 kCreateEnchantment_Relay		= 0x006899C0; //this is what actually does the work

__declspec(naked) void CreateEnchantment_Entry(void)
{
		__asm
		{
				call		[kCreateEnchantment_Relay]	// (returns EnchantmentItem*)
				add			esp, 0x10					// cdecl call, do manual cleanup

				sub			esp, 4						// shift stack pointer down to reserve a placeholder		sub 0x04

				pushad									// (save registers to stack)								sub 0x20
				push		eax							// push result from the previous call as second argument	sub 0x04
				mov			eax, [esp + 0xC + 0x28]		// get original param back from stack...     [original offset + 0x28]
				push		eax							// ...and push as first argument
				call 		HandleCraftedEnchantment	// stdcall so we don't need to do work
				mov  		[esp + 0x20], eax 			// save result into placeholder
				popad									// (restore registers [shifts esp back up 0x20])

				pop			eax							// pop result from placeholder

				jmp  		[kCreateEnchantment_Hook_Return]
		}
};


void CraftHook_Internal_Commit(void)
{
	WriteRelJump(kCreateEnchantment_Hook_Enter, (UInt32)CreateEnchantment_Entry);
}


};
