#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <UnrealDef.hpp>

using namespace RC;

#define STR_MERGE_IMPL(x, y) x##y
#define STR_MERGE(x, y) STR_MERGE_IMPL(x, y)
#define MAKE_PAD(size) char STR_MERGE(pad_, __COUNTER__)[size]
#define DEFINE_MEMBER_N(x, offset)                                                                                                                                                                     \
    struct                                                                                                                                                                                             \
    {                                                                                                                                                                                                  \
        MAKE_PAD(offset);                                                                                                                                                                              \
        x;                                                                                                                                                                                             \
    }

class ASpartaCharacter : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(UObject *WeaponsComponent, 0x6B8);
    };
};

class UGA_ActiveBlock_C : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(double ActivationTime, 0x6F8);
    };
};

struct FScalableFloat
{
public:
    float Value;
};

struct FGameplayEffectModifierMagnitude
{
public:
    union
    {
        DEFINE_MEMBER_N(FScalableFloat ScalableFloatMagnitude, 0x8);
    };
};

class UGameplayEffect : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(FGameplayEffectModifierMagnitude DurationMagnitude, 0x38);
    };
};

class UGA_Harden_Original_C : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(float PerfectStoneFormDuration, 0x5BC);
    };
};

class UAnimMontage : public UObject
{
};

class UAnimNotifyState : public UObject
{
};

struct FAnimLinkableElement
{
    char                     pad_0[8];
    class UAnimMontage      *LinkedMontage;
    int32_t                  SlotIndex;
    int32_t                  SegmentIndex;
    uint8_t                  LinkMethod;
    uint8_t                  CachedLinkMethod;
    float                    SegmentBeginTime;
    float                    SegmentLength;
    float                    LinkValue;
    class UAnimSequenceBase *LinkedSequence;
};

struct FAnimNotifyEvent : public FAnimLinkableElement
{
    float                   TriggerTimeOffset;
    float                   EndTriggerTimeOffset;
    float                   TriggerWeightThreshold;
    FName                   NotifyName;
    class UAnimNotify      *Notify;
    class UAnimNotifyState *NotifyStateClass;
    float                   Duration;
    FAnimLinkableElement    EndLink;
    bool                    bConvertedFromBranchingPoint;
    uint8_t                 MontageTickType;
    bool                    bNotifyEnabled;
    float                   NotifyTriggerChance;
    uint8_t                 NotifyFilterType;
    int32_t                 NotifyFilterLOD;
    bool                    bCanBeFilteredViaRequest;
    bool                    bTriggerOnDedicatedServer;
    bool                    bTriggerOnFollower;
    int32_t                 TrackIndex;
    char                    pad_168[16];
};

class UAnimSequenceBase : public UObject
{
public:
    char                     pad_0[128];
    TArray<FAnimNotifyEvent> Notifies;
};

class ASpartaAICharacter : public ASpartaCharacter
{
};

class UBPC_EnemyHealthWidget_C : public UObject
{
};

class ABP_AICharacter_C : public ASpartaAICharacter
{
};

class UBPC_AttackWarningInvoker_C : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(ABP_AICharacter_C *OwnerEnemy, 0xC0);
        DEFINE_MEMBER_N(AActor *OwnerActor, 0xC8);
        DEFINE_MEMBER_N(class USceneComponent *InvokerComponent, 0x00D0);
        DEFINE_MEMBER_N(ASpartaCharacter *PlayerCharacter, 0x00D8);
    };
};

class UAnimInstance : public UObject
{
};

struct DelayedAction
{
    std::chrono::steady_clock::time_point trigger_time;
    std::function<void()>                 callback;
};

struct WindowTime
{
    float start;
    float end;
    bool  isDanger;
};

class APlayerController : public UObject
{
public:
    union
    {
        DEFINE_MEMBER_N(UObject *AcknowledgedPawn, 0x0350);
    };
};

struct FAnimNotifyEventReference
{
    char                    pad_0[24];
    class UMirrorDataTable *MirrorTable;
    UObject                *NotifySource;
    char                    pad_40[8];
};

struct NotifyBeginParams
{
    class USkeletalMeshComponent    *MeshComp;
    class UAnimSequenceBase         *Animation;
    float                            TotalDuration;
    const FAnimNotifyEventReference &EventReference;
};

struct NotifyEndParams
{
    class USkeletalMeshComponent    *MeshComp;
    class UAnimSequenceBase         *Animation;
    const FAnimNotifyEventReference &EventReference;
};

class ParryIndicator : public CppUserModBase
{
private:
    std::chrono::steady_clock::time_point lastCheck        = std::chrono::steady_clock::now();
    std::unordered_map<uint64_t, int32_t> activeIndicators = {};
    std::unordered_set<uint64_t>          activeDangers    = {};

    std::optional<std::pair<int, int>> hookClientRestart       = std::nullopt;
    std::optional<std::pair<int, int>> hookUpdateAttackWarning = std::nullopt;
    std::optional<std::pair<int, int>> hookOnDeath             = std::nullopt;
    std::optional<std::pair<int, int>> hookOnDangerStart       = std::nullopt;
    std::optional<std::pair<int, int>> hookOnDangerEnd         = std::nullopt;

    bool sealDetected      = false;
    bool isModifierEnabled = false;
    bool isShowMenu        = true;

    float blockActivationTime            = 0.f;
    float blockDurationMagnitude         = 0.f;
    float hardenPerfectStoneFormDuration = 0.f;
    float parryLinkValue                 = 0.f;
    float parryDuration                  = 0.f;

    float castDelay   = 0.f;
    float guardWindow = 0.f;
    float modifier    = 0.f;

    UFunction *GetAllWeapons                 = nullptr;
    UFunction *GetAnimInstance               = nullptr;
    UFunction *GetCurrentActiveMontage       = nullptr;
    UFunction *Montage_GetPosition           = nullptr;
    UFunction *SpawnParryableAttackIndicator = nullptr;
    UFunction *SpawnWarningIndicator         = nullptr;

    APlayerController *playerController = nullptr;

public:
    auto render() -> void
    {
        ImGui::SetNextWindowSize({ 320, 0 }, ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2{ 16, 48 }, ImGuiCond_Once);
        ImGui::Begin("Parry Indicator's Menu", &isShowMenu, ImGuiWindowFlags_NoCollapse);
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::BeginTabBar("##TabBar"))
        {
            if (ImGui::BeginTabItem("Home"))
            {
                ImGui::Spacing();
                ImGui::Checkbox("Enable Timing Modifier (?)", &isModifierEnabled);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary)) ImGui::SetTooltip("Changing below slider's value will change how Parry Indicator timing, Negative: Sooner | Positive: Later");
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Current Value: %f (s)", modifier);
                ImGui::Spacing();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderFloat("##Timing Slider", &modifier, -2, 2);
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

public:
    ParryIndicator() : CppUserModBase()
    {
        ModName        = STR("ParryIndicator");
        ModVersion     = STR("1.1");
        ModDescription = STR("A mod that shows a visual indicator during the actual timing window where an enemy attack can be parried, perfect-blocked, or hardened.");
        ModAuthors     = STR("fakekey2k");

        register_tab(STR("Parry Indicator"),
                     [](CppUserModBase *mod)
                     {
                         UE4SS_ENABLE_IMGUI()
                         dynamic_cast<ParryIndicator *>(mod)->render();
                     });

        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Loading...\n"));
    }

    ~ParryIndicator() override
    {
        if (hookClientRestart)
        {
            UObjectGlobals::UnregisterHook(STR("/Script/Engine.PlayerController:ClientRestart"), hookClientRestart.value());
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unhooked ClientRestart: {} | {}\n"), hookClientRestart.value().first, hookClientRestart.value().second);
        }

        if (hookUpdateAttackWarning)
        {
            UObjectGlobals::UnregisterHook(STR("/Game/Sparta/Core/AI/Components/BPC_AttackWarningInvoker.BPC_AttackWarningInvoker_C:UpdateAttackWarning"), hookUpdateAttackWarning.value());
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unhooked UpdateAttackWarning: {} | {}\n"), hookUpdateAttackWarning.value().first, hookUpdateAttackWarning.value().second);
        }

        if (hookOnDeath)
        {
            UObjectGlobals::UnregisterHook(STR("/Game/Sparta/Core/AI/Components/BPC_AttackWarningInvoker.BPC_AttackWarningInvoker_C:OnDeathStarted_Event"), hookOnDeath.value());
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unhooked OnDeath: {} | {}\n"), hookOnDeath.value().first, hookOnDeath.value().second);
        }

        if (hookOnDangerStart)
        {
            UObjectGlobals::UnregisterHook(STR("/Game/Sparta/Core/Animations/ANS/ANS_UnparryableAttackWarning.ANS_UnparryableAttackWarning_C:Received_NotifyBegin"), hookOnDangerStart.value());
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unhooked OnDangerStart: {} | {}\n"), hookOnDangerStart.value().first, hookOnDangerStart.value().second);
        }

        if (hookOnDangerEnd)
        {
            UObjectGlobals::UnregisterHook(STR("/Game/Sparta/Core/Animations/ANS/ANS_UnparryableAttackWarning.ANS_UnparryableAttackWarning_C:Received_NotifyEnd"), hookOnDangerEnd.value());
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unhooked OnDangerEnd: {} | {}\n"), hookOnDangerEnd.value().first, hookOnDangerEnd.value().second);
        }

        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Unloaded!\n"));
    }

    auto TryDetectSeal() -> void
    {
        if (sealDetected || !playerController)
        {
            return;
        }

        if (!playerController->AcknowledgedPawn)
        {
            return;
        }

        auto localPlayer = playerController->AcknowledgedPawn;
        if (!localPlayer || localPlayer->GetClassPrivate()->GetFName().ToString() != STR("BP_PlayerCharacter_C"))
        {
            return;
        }

        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Getting current seal...\n"));

        auto weaponComponent = ((ASpartaCharacter *)localPlayer)->WeaponsComponent;
        if (!weaponComponent)
        {
            return;
        }

        if (!GetAllWeapons)
        {
            GetAllWeapons = weaponComponent->GetFunctionByName(STR("GetAllWeapons"));
        }

        if (GetAllWeapons)
        {
            struct
            {
                TArray<UObject *> Return;
            } params;

            weaponComponent->ProcessEvent(GetAllWeapons, &params);

            for (const auto &weapon : params.Return)
            {
                auto weaponName = weapon->GetClassPrivate()->GetFName().ToString();
                if (weaponName == STR("WP_TarnishedSeal_C"))
                {
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Found: {}\n"), weaponName);

                    castDelay    = blockActivationTime;
                    guardWindow  = blockDurationMagnitude;
                    sealDetected = true;
                }
                else if (weaponName == STR("WP_StoneSeal_C"))
                {
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Found: {}\n"), weaponName);

                    castDelay    = 0.f;
                    guardWindow  = hardenPerfectStoneFormDuration;
                    sealDetected = true;
                }
                else if (weaponName == STR("WP_InfiniteSeal_C"))
                {
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Found: {}\n"), weaponName);

                    castDelay    = parryLinkValue;
                    guardWindow  = parryDuration;
                    sealDetected = true;
                }
                else if (weaponName == STR("WP_SlayerSeal_C"))
                {
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Found: {}\n"), weaponName);

                    castDelay    = 0.f;
                    guardWindow  = 0.f;
                    sealDetected = true;
                }
            }

            if (sealDetected)
            {
                Output::send<LogLevel::Warning>(STR("[ParryIndicator] CastDelay: {} | Window: {} has set!\n"), castDelay, guardWindow);
            }
        }
    }

    auto on_update() -> void override
    {
        auto now = std::chrono::steady_clock::now();
        if (now >= lastCheck + std::chrono::milliseconds(5000))
        {
            if (!sealDetected && playerController)
            {
                TryDetectSeal();
            }

            lastCheck = now;
        }
    }

    auto ParryIndicatorUnHook() -> void
    {
        activeIndicators = {};
        activeDangers    = {};
    }

    auto LoadClassDefault(const StringType &assetPath, const StringType &klassPath) -> std::optional<UObject *>
    {
        try
        {
            Unreal::FSoftObjectPath asset{ FString{ assetPath } };
            asset.TryLoad();

            auto klass = UObjectGlobals::StaticFindObject<UClass *>(nullptr, nullptr, klassPath);
            if (!klass)
            {
                Output::send<LogLevel::Error>(STR("[ParryIndicator] Class unavailable: {}\n"), klassPath);
                return std::nullopt;
            }

            auto cdo = klass->GetClassDefaultObject();
            if (cdo)
            {
                return cdo.Get();
            }

            Output::send<LogLevel::Error>(STR("[ParryIndicator] ClassDefault unavailable: {}\n"), klassPath);
        }
        catch (...)
        {
        }

        return std::nullopt;
    }

    auto LoadMontage(const StringType &montagePath) -> std::optional<UObject *>
    {
        try
        {
            Unreal::FSoftObjectPath asset{ FString{ montagePath } };
            asset.TryLoad();

            auto montage = UObjectGlobals::StaticFindObject<UObject *>(nullptr, nullptr, montagePath);
            if (montage)
            {
                return montage;
            }
            else
            {
                Output::send<LogLevel::Error>(STR("[ParryIndicator] Montage unavailable: {}\n"), montagePath);
                return std::nullopt;
            }
        }
        catch (...)
        {
        }

        return std::nullopt;
    }

    auto IsParryNotifyState(UObject *state) -> bool
    {
        if (!state)
        {
            return false;
        }

        auto wrapper = state->GetValuePtrByPropertyNameInChain<UObject *>(STR("GameplayEffectClass"));
        if (!wrapper)
        {
            return false;
        }

        auto gameplayEffectClass = *wrapper;
        if (!gameplayEffectClass)
        {
            return false;
        }

        auto fullName = gameplayEffectClass->GetFullName();
        return fullName.contains(STR("GE_Parry_C"));
    }

    auto GetHitCheckWindows(UAnimMontage *montage) -> std::vector<WindowTime>
    {
        std::vector<WindowTime> windows = {};

        for (const auto &notify : ((UAnimSequenceBase *)montage)->Notifies)
        {
            if (notify.NotifyStateClass)
            {
                auto stateClass = notify.NotifyStateClass->GetFName().ToString();
                if (stateClass.contains(STR("HitCheck")))
                {
                    auto start    = notify.LinkValue;
                    auto duration = notify.Duration;
                    windows.push_back({
                        start,                                                   //
                        start + duration,                                        //
                        activeDangers.contains((uint64_t)montage) ? true : false //
                    });
                }
            }
        }

        return windows;
    }

    auto ParryIndicatorHook() -> void
    {
        ParryIndicatorUnHook();

        auto activeBlock = LoadClassDefault(STR("/Game/Sparta/Core/Characters/Player/Common/Abilities/ActiveBlock/GA_ActiveBlock.GA_ActiveBlock"),
                                            STR("/Game/Sparta/Core/Characters/Player/Common/Abilities/ActiveBlock/GA_ActiveBlock.GA_ActiveBlock_C"));
        if (activeBlock)
        {
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] ActiveBlock.ActivationTime: {}\n"), ((UGA_ActiveBlock_C *)activeBlock.value())->ActivationTime);
            blockActivationTime = ((UGA_ActiveBlock_C *)activeBlock.value())->ActivationTime;
        }

        auto perfectBlock = LoadClassDefault(STR("/Game/Sparta/Core/Effects/GE_ActiveBlock_PerfectBlock.GE_ActiveBlock_PerfectBlock"),
                                             STR("/Game/Sparta/Core/Effects/GE_ActiveBlock_PerfectBlock.GE_ActiveBlock_PerfectBlock_C"));
        if (perfectBlock)
        {
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] PerfectBlock.DurationMagnitude: {}\n"), ((UGameplayEffect *)perfectBlock.value())->DurationMagnitude.ScalableFloatMagnitude.Value);
            blockDurationMagnitude = ((UGameplayEffect *)perfectBlock.value())->DurationMagnitude.ScalableFloatMagnitude.Value;
        }

        auto harden = LoadClassDefault(STR("/Game/Sparta/Core/Characters/Player/Common/Abilities/StoneForm/GA_Harden_Original.GA_Harden_Original"),
                                       STR("/Game/Sparta/Core/Characters/Player/Common/Abilities/StoneForm/GA_Harden_Original.GA_Harden_Original_C"));
        if (harden)
        {
            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Harden.PerfectStoneFormDuration: {}\n"), ((UGA_Harden_Original_C *)harden.value())->PerfectStoneFormDuration);
            hardenPerfectStoneFormDuration = ((UGA_Harden_Original_C *)harden.value())->PerfectStoneFormDuration;
        }

        auto montage = LoadMontage(STR("/Game/Sparta/Characters/Shells/_Shared/Animation/Parry/AM_Shared_Actions_InfSeal_Parry_01_A.AM_Shared_Actions_InfSeal_Parry_01_A"));
        if (montage)
        {
            for (const auto &notify : ((UAnimSequenceBase *)montage.value())->Notifies)
            {
                auto state = notify.NotifyStateClass;
                if (IsParryNotifyState(state))
                {
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Parry | LinkValue: {} | Duration: {}\n"), notify.LinkValue, notify.Duration);
                    parryLinkValue = notify.LinkValue;
                    parryDuration  = notify.Duration;
                }
            }
        }

        if (!hookOnDeath)
        {
            hookOnDeath = UObjectGlobals::RegisterHook(
                STR("/Game/Sparta/Core/AI/Components/BPC_AttackWarningInvoker.BPC_AttackWarningInvoker_C:OnDeathStarted_Event"),
                [&](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData)
                {
                    auto invoker   = (UBPC_AttackWarningInvoker_C *)Context.Context;
                    auto invokerId = (uint64_t)invoker;

                    auto removed = activeIndicators.erase(invokerId);
                    Output::send<LogLevel::Warning>(STR("[ParryIndicator] Removed {} Dead: {}\n"), removed, invoker->OwnerEnemy->GetFName().ToString());
                },
                [](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData) {}, nullptr);

            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Hooked OnDeath: {} | {}\n"), hookOnDeath.value().first, hookOnDeath.value().second);
        }

        if (!hookUpdateAttackWarning)
        {
            hookUpdateAttackWarning = UObjectGlobals::RegisterHook(
                STR("/Game/Sparta/Core/AI/Components/BPC_AttackWarningInvoker.BPC_AttackWarningInvoker_C:UpdateAttackWarning"),
                [&](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData)
                {
                    if (!hookOnDangerStart)
                    {
                        hookOnDangerStart = UObjectGlobals::RegisterHook(
                            STR("/Game/Sparta/Core/Animations/ANS/ANS_UnparryableAttackWarning.ANS_UnparryableAttackWarning_C:Received_NotifyBegin"),
                            [&](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData)
                            {
                                auto params   = Context.GetParams<NotifyBeginParams>();
                                auto dangerId = (uint64_t)params.Animation;
                                activeDangers.insert(dangerId);
                            },
                            [](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData) {}, nullptr);

                        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Hooked OnDangerStart: {} | {}\n"), hookOnDangerStart.value().first, hookOnDangerStart.value().second);
                    }
                    if (!hookOnDangerEnd)
                    {
                        hookOnDangerEnd = UObjectGlobals::RegisterHook(
                            STR("/Game/Sparta/Core/Animations/ANS/ANS_UnparryableAttackWarning.ANS_UnparryableAttackWarning_C:Received_NotifyEnd"),
                            [&](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData)
                            {
                                auto params   = Context.GetParams<NotifyEndParams>();
                                auto dangerId = (uint64_t)params.Animation;
                                auto removed  = activeDangers.erase(dangerId);
                                Output::send<LogLevel::Warning>(STR("[ParryIndicator] Removed {} DangerAtk: {}\n"), removed, params.Animation->GetFName().ToString());
                            },
                            [](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData) {}, nullptr);

                        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Hooked OnDangerEnd: {} | {}\n"), hookOnDangerEnd.value().first, hookOnDangerEnd.value().second);
                    }

                    auto invoker   = (UBPC_AttackWarningInvoker_C *)Context.Context;
                    auto invokerId = (uint64_t)invoker;
                    auto enemy     = invoker->OwnerEnemy;

                    if (!GetAnimInstance)
                    {
                        GetAnimInstance = enemy->GetFunctionByNameInChain(STR("GetAnimInstance"));
                    }

                    struct
                    {
                        UAnimInstance *Return;
                    } params;

                    enemy->ProcessEvent(GetAnimInstance, &params);

                    if (!params.Return)
                    {
                        activeIndicators[invokerId] = -1;
                        return;
                    }

                    auto animInstance = params.Return;
                    if (!GetCurrentActiveMontage)
                    {
                        GetCurrentActiveMontage = animInstance->GetFunctionByNameInChain(STR("GetCurrentActiveMontage"));
                    }

                    struct
                    {
                        UAnimMontage *Return;
                    } params_2;

                    animInstance->ProcessEvent(GetCurrentActiveMontage, &params_2);

                    if (!params_2.Return)
                    {
                        activeIndicators[invokerId] = -1;
                        return;
                    }

                    auto montage = params_2.Return;
                    if (!Montage_GetPosition)
                    {
                        Montage_GetPosition = animInstance->GetFunctionByNameInChain(STR("Montage_GetPosition"));
                    }

                    struct
                    {
                        UAnimMontage *Montage;
                        float         Return;
                    } params_3;

                    params_3.Montage = montage;
                    animInstance->ProcessEvent(Montage_GetPosition, &params_3);

                    auto position = params_3.Return;
                    auto windows  = GetHitCheckWindows(montage);

                    auto activeWindowIndex = -1;
                    bool isDanger          = false;

                    for (int index = 0; index < windows.size(); index++)
                    {
                        auto w        = windows[index];
                        auto earliest = w.start - (castDelay + guardWindow) + (isModifierEnabled ? modifier : 0.f);
                        auto latest   = w.end - castDelay + (isModifierEnabled ? modifier : 0.f);

                        if (position > earliest && position < latest)
                        {
                            activeWindowIndex = index;
                            isDanger          = w.isDanger;
                            break;
                        }
                    }

                    auto it                  = activeIndicators.find(invokerId);
                    auto previousWindowIndex = (it != activeIndicators.end()) ? it->second : -1;

                    if (activeWindowIndex != -1 && previousWindowIndex != activeWindowIndex)
                    {
                        auto wrapper = enemy->GetValuePtrByPropertyNameInChain<UBPC_EnemyHealthWidget_C *>(STR("Enemy Health Widget"));
                        if (wrapper)
                        {
                            auto widget = *wrapper;
                            if (widget)
                            {
                                if (!SpawnParryableAttackIndicator)
                                {
                                    SpawnParryableAttackIndicator = widget->GetFunctionByNameInChain(STR("SpawnParryableAttackIndicator"));
                                }

                                if (!SpawnWarningIndicator)
                                {
                                    SpawnWarningIndicator = widget->GetFunctionByNameInChain(STR("SpawnWarningIndicator"));
                                }

                                struct
                                {
                                    FName Socket;
                                } params_4;

                                params_4.Socket = FName{ STR("Socket_Target") };

                                if (!isDanger)
                                {
                                    widget->ProcessEvent(SpawnParryableAttackIndicator, &params_4);
                                }
                                else
                                {
                                    widget->ProcessEvent(SpawnWarningIndicator, &params_4);
                                }
                            }
                        }

                        activeIndicators[invokerId] = activeWindowIndex;
                    }
                    else if (activeWindowIndex == -1)
                    {
                        activeIndicators[invokerId] = -1;
                    }
                },
                [](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData) {}, nullptr);

            Output::send<LogLevel::Warning>(STR("[ParryIndicator] Hooked UpdateAttackWarning: {} | {}\n"), hookUpdateAttackWarning.value().first, hookUpdateAttackWarning.value().second);
        }
    }

    auto on_unreal_init() -> void override
    {
        hookClientRestart = UObjectGlobals::RegisterHook(
            STR("/Script/Engine.PlayerController:ClientRestart"), [](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData) {},
            [&](Unreal::UnrealScriptFunctionCallableContext &Context, void *CustomData)
            {
                sealDetected     = false;
                playerController = (APlayerController *)Context.Context;

                ParryIndicatorHook();
            },
            nullptr);

        Output::send<LogLevel::Warning>(STR("[ParryIndicator] Hooked ClientRestart: {} | {}\n"), hookClientRestart.value().first, hookClientRestart.value().second);
    }
};

#define PARRY_INDICATOR_API __declspec(dllexport)
extern "C" {
PARRY_INDICATOR_API CppUserModBase *start_mod()
{
    return new ParryIndicator();
}

PARRY_INDICATOR_API void uninstall_mod(CppUserModBase *mod)
{
    delete mod;
}
}