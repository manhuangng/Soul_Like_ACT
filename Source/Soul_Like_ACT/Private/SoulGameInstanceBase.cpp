// Fill out your copyright notice in the Description page of Project Settings.


#include "SoulGameInstanceBase.h"
#include "SoulSaveGame.h"
#include "Item/ItemBasic.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SoulAssetManager.h"

USoulGameInstanceBase::USoulGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{}

void USoulGameInstanceBase::GetItemsIDWithType(const TArray<FPrimaryAssetType> ItemTypes, TMap<FPrimaryAssetId, FSoulItemData>& OutpItems)
{
	TArray<FPrimaryAssetId> TempAssetIds;
	USoulAssetManager& CurrentAssetManager = USoulAssetManager::Get();

	for (auto AssetType : ItemTypes)
	{
		CurrentAssetManager.GetPrimaryAssetIdList(AssetType, TempAssetIds);
		for (auto TempObj : TempAssetIds) 
		{
			OutpItems.Add(TempObj);
		}
	}
}

void USoulGameInstanceBase::AddDefaultInventory(USoulSaveGame* SaveGame, bool bRemoveExtra)
{
	// If we want to remove extra, clear out the existing inventory
 	if (bRemoveExtra)
 	{
 		SaveGame->InventoryData.Reset();
 	} 
 	// Now add the default inventory, this only adds if not already in hte inventory
 	for (const TPair<FPrimaryAssetId, FSoulItemData>& Pair : DefaultInventory)
 	{
 		if (!SaveGame->InventoryData.Contains(Pair.Key))
 		{
 			SaveGame->InventoryData.Add(Pair.Key, Pair.Value);
 		}
 	}
}

bool USoulGameInstanceBase::IsValidItemSlot(FSoulItemSlot ItemSlot) const
{
return false;
}

USoulSaveGame* USoulGameInstanceBase::GetCurrentSaveGame()
{
	return nullptr;
}

void USoulGameInstanceBase::SetSavingEnabled(bool bEnabled)
{
}

bool USoulGameInstanceBase::LoadOrCreateSaveGame()
{
	return false;
}

bool USoulGameInstanceBase::WriteSaveGame()
{
	return false;
}

void USoulGameInstanceBase::ResetSaveGame()
{
	bool bWasSavingEnabled = bSavingEnabled;
	bSavingEnabled = false;
	LoadOrCreateSaveGame();
	bSavingEnabled = bWasSavingEnabled;
}