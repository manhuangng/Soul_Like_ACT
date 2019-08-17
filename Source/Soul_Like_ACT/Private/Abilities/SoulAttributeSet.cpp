// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/SoulAttributeSet.h"
#include "GameplayEffect.h"
#include "SoulCharacterBase.h"
#include "GameplayEffectExtension.h"


USoulAttributeSet::USoulAttributeSet()
	: Health(1.f)
	, MaxHealth(1.f)
	, Posture(0.f)
	, MaxPosture(1.f)
	, PostureStrength(0.0f)
	, DefensePower(0.f)
	, MeleeRange(0.f)
	, AttackPower(0.0f)
	, AttackSpeed(0.0f)
	, Leech(0.0f)
	, PostureCrumble(0.0f)
	, MoveSpeed(.1f)
	, CriticalStrike(5.0f)
	, CriticalMulti(50.f)
	, Damage(0.0f)
	, PostureDamage(0.0f)
{
}

void USoulAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();

	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();

	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void USoulAttributeSet::GetHitEventFromTagContainer(const FGameplayTagContainer& InTagContainer, EParryResult& OutParryResult, bool& IsCritical, bool& IsStun)
{
	static FGameplayTagContainer StunTagContainer;
	if (!StunTagContainer.IsValid())
	{
		StunTagContainer.AddTagFast(FGameplayTag::RequestGameplayTag(FName("Event.Hit.Parry.Failed"), true));
		StunTagContainer.AddTagFast(FGameplayTag::RequestGameplayTag(FName("Event.Hit.GetHit"), true));
	}
	
	static FGameplayTag ParryTag;
	if(!ParryTag.IsValid()) ParryTag = FGameplayTag::RequestGameplayTag(FName("Event.Hit.Parry"), true);

	static FGameplayTag CriticalTag;
	if (!CriticalTag.IsValid()) FGameplayTag::RequestGameplayTag(FName("Event.Hit.Critical"), true);
	
	bool bFindParryTag = false;
	//Set ParryResult
	for (TArray<FGameplayTag>::TConstIterator TagIter = InTagContainer.CreateConstIterator(); TagIter; ++TagIter)
	{
		LOG_FUNC_NORMAL(TagIter->ToString());

		if (!bFindParryTag && TagIter->MatchesTag(ParryTag))
		{
			bFindParryTag = true;

			TArray<FString> TempParsedTag;
			TagIter->ToString().ParseIntoArray(TempParsedTag, *FString("."));
			
			if (TempParsedTag.Num() > 0)
			{
				FString ParryTagLeaf = TempParsedTag.Last();
				if (ParryTagLeaf == "Perfect")
				{
					OutParryResult = EParryResult::Perfect;


				}
				else if (ParryTagLeaf == "Normal")
				{
					OutParryResult = EParryResult::Normal;
				}
				else if (ParryTagLeaf == "Failed")
				{
					OutParryResult = EParryResult::Failed;
				}
				else if (ParryTagLeaf == "Unguard")
				{
					OutParryResult = EParryResult::Unguard;
				}
			}
		}
		if (!IsStun && TagIter->MatchesAnyExact(StunTagContainer))
		{
			IsStun = true;
		}
		if (!IsCritical && TagIter->MatchesTagExact(CriticalTag))
		{
			IsCritical = true;
		}
	}

	if (!bFindParryTag) OutParryResult = EParryResult::Unguard;
}

void USoulAttributeSet::PreAttributeChange(const FGameplayAttribute & Attribute, float & NewValue)
{
	// This is called whenever attributes change, so for max health/mana we want to scale the current totals to match
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
}

void USoulAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData & Data)
{
	Super::PostGameplayEffectExecute(Data);

	LOG_FUNC_SUCCESS();

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	//Dynamic tags from GE
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	// Compute the delta between old and new, if it is available
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	ASoulCharacterBase* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<ASoulCharacterBase>(TargetActor);
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Get the Source actor
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		ASoulCharacterBase* SourceCharacter = nullptr;
		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn* Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<ASoulCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<ASoulCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		// Try to extract a hit result
		FHitResult HitResult;
		if (Context.GetHitResult())
		{
			HitResult = *Context.GetHitResult();
		}

		// Store a local copy of the amount of damage done and clear the damage attribute
		const float LocalDamageDone = GetDamage();
		SetDamage(0.f);

		//Health Change
		if (LocalDamageDone > 0)
		{
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, GetMaxHealth()));
		}

		//Handle damage notifications
		if (TargetCharacter)
		{
			FGameplayTagContainer LocalContainer;
			Data.EffectSpec.GetAllAssetTags(LocalContainer);

			//Get Hit results for HitEvents
			bool bIsCritic, bIsStun;
			EParryResult ParryResult;
			GetHitEventFromTagContainer(LocalContainer, ParryResult, bIsCritic, bIsStun);

			if (ParryResult == EParryResult::Normal)
			{
				if(TargetCharacter->OnParryNormal.IsBound())
					TargetCharacter->OnParryNormal.Broadcast(SourceActor, TargetActor, HitResult);
			}
			else if (ParryResult == EParryResult::Perfect)
			{
				if (TargetCharacter->OnParrySucced.IsBound())
					TargetCharacter->OnParrySucced.Broadcast(SourceActor, TargetActor, HitResult);
			}
			else if (ParryResult == EParryResult::Failed)
			{

				if (TargetCharacter->OnParryFailed.IsBound())
					TargetCharacter->OnParryFailed.Broadcast(SourceActor, TargetActor, HitResult);
			}

			if (LocalContainer.HasTagExact(FGameplayTag::RequestGameplayTag(FName{ "Damage.Dot" }, true)))
			{
				TargetCharacter->HandleDotDamage(LocalDamageDone, bIsCritic, bIsStun, HitResult, SourceTags, SourceCharacter, SourceActor);
			}
			else
			{
				if (GetHealth() <= 0.f)
					(Cast<ASoulCharacterBase>(SourceActor))->Notify_OnMeleeKill(SourceActor, TargetActor, HitResult);

				TargetCharacter->HandleDamage(LocalDamageDone, bIsCritic, ParryResult, bIsStun, HitResult, SourceTags, SourceCharacter, SourceActor);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetPostureDamageAttribute())
	{
		// Get the Source actor
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		ASoulCharacterBase* SourceCharacter = nullptr;
		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn * Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<ASoulCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<ASoulCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		// Try to extract a hit result
		FHitResult HitResult;
		if (Context.GetHitResult())
		{
			HitResult = *Context.GetHitResult();
		}

		/**
		 * Data.EffectSpec == FGameplayEffectSpec
		 * Data.EffectSpec.DynamicAssetTags
		 */
		bool bIsCritic = Data.EffectSpec.DynamicAssetTags.HasTagExact(FGameplayTag::RequestGameplayTag(FName{ "Event.Montage.Shared.Critical" }, true));

		// Store a local copy of the amount of damage done and clear the damage attribute
		const float LocalPostureDamageDone = GetPostureDamage();
		SetPostureDamage(0.f);

		if (LocalPostureDamageDone > 0)
		{
			// Apply the health change and then clamp it
			const float OldPosture = GetPosture();
			SetPosture(FMath::Clamp(OldPosture + LocalPostureDamageDone, 0.0f, GetMaxHealth()));

			if (TargetCharacter)
			{
				// This is proper damage
				TargetCharacter->HandlePostureDamage(LocalPostureDamageDone, bIsCritic, HitResult, SourceTags, SourceCharacter, SourceActor);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Handle other health changes such as from healing or direct modifiers
		// First clamp it
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetPostureAttribute())
	{
		SetPosture(FMath::Clamp(GetPosture(), 0.0f, GetMaxPosture()));
	}
	else if (Data.EvaluatedData.Attribute == GetLeechAttribute())
	{
		SetLeech(FMath::Clamp(GetLeech (), 0.0f, 100.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackSpeedAttribute())
	{
		SetLeech(FMath::Clamp(GetAttackSpeed(), 0.0f, 999.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Clamp(GetMoveSpeed(), 0.0f, 3000.f));
	}
	else if (Data.EvaluatedData.Attribute == GetMeleeRangeAttribute())
	{
		SetMeleeRange(FMath::Clamp(GetMeleeRange(), 0.0f, 500.f));
	}
	//Posture Strength
	else if (Data.EvaluatedData.Attribute == GetPostureStrengthAttribute())
	{
		SetPostureStrength(FMath::Clamp(GetPostureStrength(), 0.0f, 9999.0f));
	}
	//DP
	else if (Data.EvaluatedData.Attribute == GetDefensePowerAttribute())
	{
		SetDefensePower(FMath::Clamp(GetDefensePower(), 0.0f, 9999.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackPowerAttribute())
	{
		SetAttackPower(FMath::Clamp(GetAttackPower(), 0.0f, 9999.0f));
	}
	//Posture Crumble
	else if (Data.EvaluatedData.Attribute == GetPostureCrumbleAttribute())
	{
		SetPostureCrumble(FMath::Clamp(GetPostureCrumble(), 0.0f, 9999.0f));
	}
	//CS
	else if (Data.EvaluatedData.Attribute == GetCriticalStrikeAttribute())
	{
		SetCriticalStrike(FMath::Clamp(GetCriticalStrike(), -999.f, 999.f));
	}
	//CM
	else if (Data.EvaluatedData.Attribute == GetCriticalMultiAttribute())
	{
		SetCriticalMulti(FMath::Clamp(GetCriticalMulti(), -999.f, 999.f));
	}
}