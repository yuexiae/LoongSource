#include "StdAfx.h"
#include "CombatActionChecker.h"
#include "RoleMgr.h"
#include "Player.h"
#include "NPC.h"
#include "Door.h"
#include "LocalPlayer.h"
#include "CombatSysUtil.h"
#include "..\WorldDefine\skill_define.h"
#include "..\WorldDefine\role_att.h"
#include "SkillMgr.h"
#include "ItemProtoData.h"
#include "MapMgr.h"
#include "TeamSys.h"
#include "ActivityPvPMgr.h"
#include "..\WorldDefine\msg_motion.h"
#include "SocialMgr.h"
#include "LocalPlayer.h"
#include "SkillMgr.h"
#include "GuildMgr.h"

const DWORD FriendPlayerMask=ETF_Self|ETF_Teammate|ETF_Guildmate|ETF_Brother|ETF_Teacher|ETF_Lover|ETF_Couple;

CombatActionChecker::CombatActionChecker(void)
{
}

CombatActionChecker::~CombatActionChecker(void)
{
}

CombatActionChecker g_combatActionChecker;
CombatActionChecker* CombatActionChecker::Inst()
{
	return &g_combatActionChecker;
}

void CombatActionChecker::Init()
{

}

void CombatActionChecker::Destroy()
{

}

bool CombatActionChecker::IfCanUseSkill( DWORD skillID,DWORD targetID,bool bShowErrMsg )
{
	const tagSkillData* pSkillData=SkillMgr::Inst()->GetSkillDataByTypeID(skillID);
	if(!P_VALID(pSkillData))
		return false;

	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//--摆摊状态下不能使用技能
	const DWORD RoleStateMask=ERS_Stall;
	if(pSelf->GetRoleState()&RoleStateMask)
		return false;

	//--被动技能不能使用
	if(pSkillData->pProto->eUseType==ESUT_Passive)
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_Passive);
		return false;
	}

    //如果已有骑乘类型buff，则先debuff，不使用技能
    if (pSkillData->pProto->nType2 == ESSTE_PetSkill)
    {
        TList<tagRoleBuff*>& buffList = RoleMgr::Inst()->GetLocalPlayer()->GetBuffList();
        tagRoleBuff* pRoleBuff = NULL;
        buffList.ResetIterator();
        while(buffList.PeekNext(pRoleBuff))
        {
            for (int i = 0; i < MAX_BUFF_PER_SKILL; ++i)
            {
                if (P_VALID(pRoleBuff) && pSkillData->pProto->dwBuffID[i] == pRoleBuff->dwBuffTypeID)
                {
                    tagNC_CancelBuff cmd;
                    cmd.dwBuffTypeID = pRoleBuff->dwBuffTypeID;
                    TObjRef<NetSession>()->Send( &cmd );

                    return false;
                }

            }
        }
    }

	//--检查自身状态
	const DWORD dwSelfStateFlag=GetRoleStateFlag(pSelf);
	if( (dwSelfStateFlag&pSkillData->pProto->dwSelfStateLimit)!=dwSelfStateFlag  )
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_SelfStateLimit);
		return false;
	}

	if(pSelf->IsSwimState())
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_SwimLimit);
		return false;
	}

	//--武器检查
	if(pSkillData->pProto->nWeaponLimit!=0
		&&pSkillData->pProto->nWeaponLimit!=pSelf->GetRWeaponType()
		&&pSkillData->pProto->nWeaponLimit!=pSelf->GetLWeaponType())
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_WeaponLimit);
		return false;
	}

	//--前置BUFF
	if(P_VALID(pSkillData->pProto->dwBuffLimitID)
		&&!P_VALID(pSelf->FindBuff(pSkillData->pProto->dwBuffLimitID)))
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_SkillBuffLimit);
		return false;
	}

	//--消耗检测
	if(pSkillData->pProto->nSkillCost[ESCT_HP]>0
		&&pSelf->GetAttribute(ERA_HP)< GetCostByType( pSelf,pSkillData,ESCT_HP ) )
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_HPNotEnough);
		return false;
	}
	if(pSkillData->pProto->nSkillCost[ESCT_MP]>0
		&&pSelf->GetAttribute(ERA_MP)< GetCostByType( pSelf,pSkillData,ESCT_MP ) )
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_MPNotEnough);
		return false;
	}	
	
	
	if(pSkillData->pProto->nSkillCost[ESCT_Rage]>0
		&&pSelf->GetAttribute(ERA_Rage)< GetCostByType( pSelf,pSkillData,ESCT_Rage ) )
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_RageNotEnough);
		return false;
	}

	if(pSkillData->pProto->eOPType==ESOPT_Explode
		&&pSkillData->pProto->fOPDist==0)//以自已为中心的爆炸技能?
	{

	}
	else
	{
		Role* pTarget=RoleMgr::Inst()->FindRole(targetID);
		if(pTarget==NULL)
		{
			if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_TargetNotExist);
			return false;
		}

		//--目标类型检查
		DWORD targetType=GetTargetType(pTarget);
		if( (pSkillData->pProto->dwTargetLimit&targetType) == 0)
		{
			if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_InvalidTarget);
			return false;
		}

		//--敌我判断
		if( (pSkillData->pProto->bFriendly&&IsFriend(pTarget,targetType))
			|| (pSkillData->pProto->bHostile&&IsEnemy(pTarget,targetType))
			|| (pSkillData->pProto->bIndependent&&IsIndependent(pTarget,targetType)) )
		{
		}
		else
		{
			if(bShowErrMsg)
			{
				if(pSkillData->pProto->bHostile)
					CombatSysUtil::Inst()->ShowActionCheckMsg((DWORD)EEC_CannotAttack);
				else
					CombatSysUtil::Inst()->ShowActionCheckMsg((DWORD)EEC_InvalidTarget);
			}
			return false;
		}

		//--目标状态检查
		const DWORD dwTargetStateFlag=GetRoleStateFlag(pTarget);
		if( (dwTargetStateFlag&pSkillData->pProto->dwTargetStateLimit)!=dwTargetStateFlag  )
		{
			if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg((DWORD)EEC_InvalidTarget);
			return false;
		}

		//--PK相关
		//自身处于保护模式时，不能对PVP模式的玩家使用技能(在战场外)
		if(pTarget!=pSelf
			&& pTarget->IS_KIND_OF(Player))
		{
			Player* pPlayer=(Player*)pTarget;
			bool bIsArena = IsInArena(pPlayer);
			if( (pSelf->GetRoleState()&ERS_Safeguard)!=0
				&& (pPlayer->GetRoleState()&ERS_PVP)!=0 && !bIsArena)
			{
				if(bShowErrMsg)
					CombatSysUtil::Inst()->ShowActionCheckMsg((DWORD)EEC_PVP);
				return false;
			}
		}

		//--目标前置BUFF
		if(P_VALID(pSkillData->pProto->dwTargetBuffLimitID)
			&&!P_VALID(pTarget->FindBuff(pSkillData->pProto->dwTargetBuffLimitID)))
		{
			if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_SkillBuffLimit);
			return false;
		}

		//--射线检测
		if(pTarget!=pSelf)
		{
			if(!RayCollideTarget(pTarget))
			{
				if(bShowErrMsg)
					CombatSysUtil::Inst()->ShowActionCheckMsg((DWORD)EEC_TargetBlocked);
				return false;
			}
		}
	}

	return true;
}

bool CombatActionChecker::IsInPVPArea( Player* pTarget )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();
	return pSelf->GetRoleState()&ERS_PVPArea
		&& pTarget->GetRoleState()&ERS_PVPArea;
}

bool CombatActionChecker::IsInSafeArea( Player* pTarget )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();
	return pSelf->GetRoleState()&ERS_SafeArea
		|| pTarget->GetRoleState()&ERS_SafeArea;
}

bool CombatActionChecker::IsInArena(Player * pTarget)
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();
	return pSelf->GetRoleState(ERS_Arena)
		&& pTarget->GetRoleState(ERS_Arena);
}

DWORD CombatActionChecker::GetTargetType( Role* pTarget )
{
	DWORD ret=0;

	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//自身
	if(pTarget==pSelf)
		ret|=ETF_Self;

	//队友
	if(TeamSys::Inst()->IsTeammate(pTarget->GetID()))
		ret|=ETF_Teammate;

	//情侣
	if(false)//todo
		ret|=ETF_Lover;

	//配偶
	if( pSelf->GetPlayerLoverID() == pTarget->GetID() )
		ret|=ETF_Couple;

	//结拜
	if(false)//todo
		ret|=ETF_Brother;

	//师徒
	if(false)//todo
		ret|=ETF_Teacher;

	//玩家
	if(pTarget->IS_KIND_OF(Player))
	{
		ret|=ETF_Player;
		//同门
		if( pSelf->GetRoleGuildID() == ((Player*)pTarget)->GetRoleGuildID() && pSelf->GetRoleGuildID() != GT_INVALID )
			ret|=ETF_Guildmate;
	}

	if(pTarget->IS_KIND_OF(NPC))
	{
		NPC* pNPC=(NPC*)pTarget;
		//宠物
		if(pNPC->IsPet())
			ret|=ETF_Pet;

		//NPC
		if(pNPC->IsNPC())
			ret|=ETF_NPC;

		//普通怪物
		if(pNPC->IsMonster())
			ret|=ETF_NormalMonster;

		//boss
		if(false)
			ret|=ETF_Boss;

		//巢穴
		if(false)
			ret|=ETF_Nest;

		const tagCreatureProto* pProto=pNPC->GetProto();
		if(P_VALID(pProto))
		{
			if(pProto->eType==ECT_GameObject)
			{
				if(pProto->nType2==EGOT_Gather)
				{
					//自然资源
					if(pProto->nType3==EGT_Mine
						||pProto->nType3==EGT_Herb)
					{
						ret|=ETF_NatuRes;
					}
					//人造资源
					else
					{
						ret|=ETF_ManRes;
					}
				}
				else if(pProto->nType2==EGOT_Normal)
				{
					ret|=ETF_NormalGameObject;
				}
				else if(pProto->nType2==EGOT_QuestInves || pProto->nType2==EGOT_CommonInves)
				{
					ret|=ETF_InvesGameObject;
				}
				else if(pProto->nType2==EGOT_QuestTalk)
				{
					ret|=ETF_QuestTalkGameObject;
				}
			}
		}

		//城门
		if(false)
			ret|=ETF_Door;

		//建筑
		if(false)
			ret|=ETF_Building;
	}

	return ret;
}

bool CombatActionChecker::IsEnemy( Role* pTarget,DWORD targetType )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//玩家
	if(targetType&ETF_Player)
	{
		ASSERT(pTarget->IS_KIND_OF(Player));
		Player* pPlayer=(Player*)pTarget;

		bool bInPVPArea=IsInPVPArea(pPlayer);
		bool bInSafeArea=IsInSafeArea(pPlayer);
		bool bInArena=IsInArena(pPlayer);

		//自已
		if(targetType&ETF_Self)
		{
			return false;
		}
		//友方
		else if(targetType&FriendPlayerMask)
		{
			//英雄乱武战场里队友也是敌人
			if(bInArena && ActivityPvPMgr::Inst()->GetPvPType()==0)
				return true;
			//群雄逐鹿队友不是敌人
			if(bInArena && ActivityPvPMgr::Inst()->GetPvPType()==1)
				return false;
			if(!bInPVPArea)
				return false;
		}
		//目标处于牢狱状态
		else if(pPlayer->GetRoleState()&ERS_PrisonArea)
		{
			return false;
		}
		//其它
		else
		{
			//战场判断敌人的优先级最高
			if( bInArena )
				return true;
			if( bInSafeArea 
				&& ( (pPlayer->GetRoleState() & ERS_PK) == 0 && (pPlayer->GetRoleState() & ERS_PKEX) == 0 ) 
				&& ( !(pSelf->GetRoleState()&ERS_PK) ) )
			{
				return false;
			}

			if(!bInPVPArea)
			{
				if(pSelf->GetRoleState()&ERS_Safeguard)
				{
					return false;
				}
				else if(pSelf->GetRoleState()&ERS_PK)
				{
					if(pPlayer->GetRoleState()&ERS_Safeguard)
						return false;
				}
				else if(pSelf->GetRoleState()&ERS_PKEX)
				{
					if( pPlayer->GetRoleState()&ERS_Safeguard || 
					  ( ( (pPlayer->GetRoleState() & ERS_PK) == 0 ) &&
					    ( (pPlayer->GetRoleState() & ERS_PKEX) == 0 ) ) )
						return false;
				}
				else
				{
					return false;
				}
			}
		}
	}

	//宠物
	if(targetType&ETF_Pet)
	{
		return false;
	}

	//NPC
	if(targetType&ETF_NPC)
	{
		if((pSelf->GetRoleState()&ERS_PK)==0)
		{
			return false;
		}
	}

	//普通怪物
	if(targetType&ETF_NormalMonster)
	{
	}

	//boss
	if(targetType&ETF_Boss)
	{
	}

	//巢穴
	if(targetType&ETF_Nest)
	{
	}

	//自然资源
	if(targetType&ETF_NatuRes)
	{
		return false;
	}

	//人造资源
	if(targetType&ETF_ManRes)
	{
		return false;
	}

	//普通可交互地物
	if(targetType&ETF_NormalGameObject)
	{
		return false;
	}

	//可调查地物
	if(targetType&ETF_InvesGameObject)
	{
		return false;
	}

	//任务对话地物
	if(targetType&ETF_QuestTalkGameObject)
	{
		return false;
	}

	//城门
	if(targetType&ETF_Door)
	{
		//todo
	}

	//建筑
	if(targetType&ETF_Building)
	{
		//todo
	}

	return true;
}

bool CombatActionChecker::IsFriend( Role* pTarget,DWORD targetType )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//玩家
	if(targetType&ETF_Player)
	{
		ASSERT(pTarget->IS_KIND_OF(Player));
		Player* pPlayer=(Player*)pTarget;

		bool bInPVPArea=IsInPVPArea(pPlayer);
		bool bInSafeArea=IsInSafeArea(pPlayer);

		//自已
		if(targetType&ETF_Self)
		{
		}
		//友方
		else if(targetType&FriendPlayerMask)
		{
			//双方都在群雄逐鹿战场时认为为队友,在英雄乱武战场时所有人都是敌人
			if( pSelf->GetRoleState(ERS_Arena) && pPlayer->GetRoleState(ERS_Arena) && ActivityPvPMgr::Inst()->GetPvPType()==1 )
				return true;
            else if( pSelf->GetRoleState(ERS_Arena) && pPlayer->GetRoleState(ERS_Arena) && ActivityPvPMgr::Inst()->GetPvPType()==0 )
                return false;

			if(TeamSys::Inst()->IsTeammate(pPlayer->GetID()) 
				|| ( pSelf->GetRoleGuildID() == pPlayer->GetRoleGuildID() && pSelf->GetRoleGuildID() != GT_INVALID )
				|| pSelf->GetPlayerLoverID() == pPlayer->GetID() )
				return true;

			if(pPlayer->GetRoleState()&ERS_PVP)
				if( TeamSys::Inst()->IsTeammate(pPlayer->GetID()) 
					|| ( pSelf->GetRoleGuildID() == pPlayer->GetRoleGuildID() && pSelf->GetRoleGuildID() != GT_INVALID )
					|| pSelf->GetPlayerLoverID() == pPlayer->GetID() )
					return true;
				else
					return false;

			//只有当双方都是行凶状态或双方都不是行凶状态才是友方
			if( pSelf->GetRoleState(ERS_PK) != pPlayer->GetRoleState(ERS_PK) )
				return false;
		}
		// 都没开行凶仍然是友方
		else if( (!pSelf->GetRoleState(ERS_PK) && !pPlayer->GetRoleState(ERS_PK)) && (!pSelf->GetRoleState(ERS_PKEX) && !pPlayer->GetRoleState(ERS_PKEX))  )
		{
		}
		//其它
		else
		{
			return false;
		}
	}

	//宠物
	if(targetType&ETF_Pet)
	{
		//todo
	}

	//NPC
	if(targetType&ETF_NPC)
	{
		if(pSelf->GetRoleState()&ERS_PK)
		{
			return false;
		}
	}

	//普通怪物
	if(targetType&ETF_NormalMonster)
	{
		return false;
	}

	//boss
	if(targetType&ETF_Boss)
	{
		return false;
	}

	//巢穴
	if(targetType&ETF_Nest)
	{
		return false;
	}

	//自然资源
	if(targetType&ETF_NatuRes)
	{
		return false;
	}

	//人造资源
	if(targetType&ETF_ManRes)
	{
		return false;
	}

	//普通可交互地物
	if(targetType&ETF_NormalGameObject)
	{
		return false;
	}

	//普通可交互地物
	if(targetType&ETF_NormalGameObject)
	{
		return false;
	}

	//可调查地物
	if(targetType&ETF_InvesGameObject)
	{
		return false;
	}

	//城门
	if(targetType&ETF_Door)
	{
		return false;
	}

	//建筑
	if(targetType&ETF_Building)
	{
		return false;
	}

	return true;
}

bool CombatActionChecker::IsIndependent( Role* pTarget,DWORD targetType )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//玩家
	if(targetType&ETF_Player)
	{
		ASSERT(pTarget->IS_KIND_OF(Player));
		Player* pPlayer=(Player*)pTarget;

		bool bInPVPArea=IsInPVPArea(pPlayer);
		bool bInSafeArea=IsInSafeArea(pPlayer);

		//自已
		if(targetType&ETF_Self)
		{
			return false;
		}
		//友方
		else if(targetType&FriendPlayerMask)
		{
			return false;
		}
		//其它
		else
		{
			if(bInPVPArea)
				return false;

			if(bInSafeArea)
				return false;

			if( (pSelf->GetRoleState(ERS_Safeguard)&&pPlayer->GetRoleState(ERS_PK))
				||(pSelf->GetRoleState(ERS_PK)&&pPlayer->GetRoleState(ERS_Safeguard)) )
			{
				return true;
			}
			else
				return false;
		}
	}

	//宠物
	if(targetType&ETF_Pet)
	{
		//todo
	}

	//NPC
	if(targetType&ETF_NPC)
	{
		if(pSelf->GetRoleState()&ERS_PK)
		{
			return false;
		}
	}

	//普通怪物
	if(targetType&ETF_NormalMonster)
	{
		return false;
	}

	//boss
	if(targetType&ETF_Boss)
	{
		return false;
	}

	//巢穴
	if(targetType&ETF_Nest)
	{
		return false;
	}

	//自然资源
	if(targetType&ETF_NatuRes)
	{
		return true;
	}

	//人造资源
	if(targetType&ETF_ManRes)
	{
		return true;
	}

	//普通可交互地物
	if(targetType&ETF_NormalGameObject)
	{
		return true;
	}

	//普通可交互地物
	if(targetType&ETF_NormalGameObject)
	{
		return true;
	}

	//可调查地物
	if(targetType&ETF_InvesGameObject)
	{
		return true;
	}

	//城门
	if(targetType&ETF_Door)
	{
		//todo
	}

	//建筑
	if(targetType&ETF_Building)
	{
		//todo
	}

	return true;
}

bool CombatActionChecker::IfCanAttack( DWORD targetID )
{
	Role* pTarget=RoleMgr::Inst()->FindRole(targetID);
	if(pTarget==NULL)
		return false;

	DWORD targetType=GetTargetType(pTarget);
	return IsEnemy(pTarget,targetType);
}

bool CombatActionChecker::IfCanUseItem( DWORD itemID,DWORD targetID,bool bShowErrMsg )
{
	const tagItemProto* pItemData=ItemProtoData::Inst()->FindItemProto(itemID);
	if(!P_VALID(pItemData))
		return false;

	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//--摆摊状态下不能使用物品
	const DWORD RoleStateMask=ERS_Stall;
	if(pSelf->GetRoleState()&RoleStateMask)
		return false;

	//--检查自身状态
	const DWORD dwSelfStateFlag=GetRoleStateFlag(pSelf);
	if( (dwSelfStateFlag&pItemData->dwStateLimit)!=dwSelfStateFlag  )
	{
		if(bShowErrMsg)CombatSysUtil::Inst()->ShowActionCheckMsg(EEC_CanNotUseItem);
		return false;
	}

	return true;
}

bool CombatActionChecker::IfCanMove(bool bShowErrMsg)
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//--检查属性是否初始化（移动速度等）
	if( !pSelf->IsAttributeInited() )
		return false;

	//--检查角色状态
	if(pSelf->IsInState(ES_Dizzy) || pSelf->IsInState(ES_Tie) || pSelf->IsInState(ES_Spor))
		return false;

	//--检查玩家状态
	const DWORD RoleStateMask=ERS_Stall;
	if(pSelf->GetRoleState()&RoleStateMask)
		return false;

	return true;
}

bool CombatActionChecker::IfCanTalkNPC(bool bShowErrMsg)
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//--检查角色状态
	if(pSelf->IsInState(ES_Dizzy) || pSelf->IsInState(ES_Tie) || pSelf->IsInState(ES_Spor))
		return false;

	//--检查玩家状态
	const DWORD RoleStateMask=ERS_Stall;
	if(pSelf->GetRoleState()&RoleStateMask)
		return false;

	return true;
}

bool CombatActionChecker::IfCanPickGroundItem( bool bShowErrMsg )
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();

	//--检查角色状态
	if(pSelf->IsInState(ES_Dizzy) || pSelf->IsInState(ES_Tie) || pSelf->IsInState(ES_Spor) || pSelf->IsDead())
		return false;

	//--检查玩家状态
	const DWORD RoleStateMask=ERS_Stall;
	if(pSelf->GetRoleState()&RoleStateMask)
		return false;

	return true;
}

bool CombatActionChecker::IfTargetIsValid(DWORD targetID,DWORD dwTargetLimit,BOOL bHostile,BOOL bFriendly,BOOL bIndependent)
{
	//--目标是否存在？
	Role* pTarget=RoleMgr::Inst()->FindRole(targetID);
	if(pTarget==NULL)
	{
		return false;
	}

	//--目标类型检查
	DWORD targetType=GetTargetType(pTarget);
	if( (dwTargetLimit&targetType) == 0)
	{
		return false;
	}

	//--敌我判断
	if( (bFriendly&&IsFriend(pTarget,targetType))
		|| (bHostile&&IsEnemy(pTarget,targetType))
		|| (bIndependent&&IsIndependent(pTarget,targetType)) )
	{
	}
	else
	{
		return false;
	}

	return true;
}

bool CombatActionChecker::RayCollideTarget( Role* pTarget )
{
	Vector3 src;
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();
	src=pSelf->GetPos();
	src.y+=pSelf->GetRoleSize().y;

	Vector3 dest=pTarget->GetPos();
	dest.y+=pTarget->GetRoleSize().y;

	Ray ray;
	ray.origin=src;
	ray.dir=dest-src;
	Vec3Normalize(ray.dir);
	ray.length=Vec3Dist(src,dest);
	if (pTarget->IsStaticClass(_T("Door")))
	{
		return (!MapMgr::Inst()->GetNavMap()->GetCollider()->RayCollideBoxAndTrn(ray, NavSceneNode::EFlag_NotDoor));
	}
	else
	{
		return !MapMgr::Inst()->GetNavMap()->GetCollider()->RayCollideBoxAndTrn(ray);
	}
}

DWORD CombatActionChecker::GetRoleStateFlag( Role* pRole )
{
	DWORD dwFlag=0;

	if(pRole->IsInState(ES_Dead))
		dwFlag|=ESF_Dead;
	else
		dwFlag|=ESF_NoDead;

	if(pRole->IsInState(ES_Dizzy))
		dwFlag|=ESF_Dizzy;
	else
		dwFlag|=ESF_NoDizzy;

	if(pRole->IsInState(ES_Tie))
		dwFlag|=ESF_Tie;
	else
		dwFlag|=ESF_NoTie;

	if(pRole->IsInState(ES_Spor))
		dwFlag|=ESF_Spor;
	else
		dwFlag|=ESF_NoSpor;

	if(pRole->IsInState(ES_Invincible))
		dwFlag|=ESF_Invincible;
	else
		dwFlag|=ESF_NoInvincible;

	if(pRole->IsInState(ES_Lurk))
		dwFlag|=ESF_Lurk;
	else
		dwFlag|=ESF_NoLurk;

	if(pRole->IsInState(ES_DisArm))
		dwFlag|=ESF_DisArm;
	else
		dwFlag|=ESF_NoDisArm;

	if(pRole->IsInState(ES_NoSkill))
		dwFlag|=ESF_NoSkill;
	else
		dwFlag|=ESF_NoNoSkill;

	if( pRole->IS_KIND_OF(Player) )
	{
		Player* pPlayer = (Player*)pRole;

		if(pPlayer->GetRoleState(ERS_Mount))
			dwFlag|=ESF_Mount;
		else
			dwFlag|=ESF_NoMount;

		//if(pPlayer->GetRoleState(ERS_Mount2))
		//	dwFlag|=ESF_Mount2;
		//else
		//	dwFlag|=ESF_NoMount2;

		if(pPlayer->GetRoleState(ERS_PrisonArea))
			dwFlag|=ESF_Prison;
		else
			dwFlag|=ESF_NoPrison;

		if(pPlayer->GetRoleState(ERS_Commerce))
			dwFlag|=ESF_Commerce;
		else
			dwFlag|=ESF_NoCommerce;
	}

	return dwFlag;
}

bool CombatActionChecker::IfCanPlayStyleAction(bool bMutual, DWORD dwFriendVal, DWORD targetID,bool bShowErrMsg)
{
	LocalPlayer* pSelf=RoleMgr::Inst()->GetLocalPlayer();
	if( !P_VALID(pSelf) || !pSelf->IsAttributeInited() )
		return false;

	// 备战状态，不可播放个性动作
	if( pSelf->IsReadyState() )
		return false;

	if(pSelf->IsSwimState())
	{
		if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_Wimming );
		return false;
	}

	// 交互动作
	if( bMutual )
	{
		// 没有目标
		if( !P_VALID(targetID) )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_RoleNotFround );
			return false;
		}

		Role* pTarget = RoleMgr::Inst()->FindRole( targetID );
		if( !P_VALID(pTarget) )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_RoleNotFround );
			return false;
		}

		// 目标是自己或不是玩家
		if( pSelf->GetID() == targetID || !pTarget->IS_KIND_OF(Player) )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			return false;
		}

		// 可攻击目标
		if( IfCanAttack( targetID ) )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			return false;
		}

		Player* pPlayer = (Player*)pTarget;

		// 目标不是异性
		if( !pPlayer->IsAttributeInited() || pSelf->GetAvatarAtt().bySex == pPlayer->GetAvatarAtt().bySex )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			return false;
		}

		// 自身状态检查
		if( ( !pSelf->IsStyleActionState() && !pSelf->IsIdleState() ) || 
			EActionState_Normal != pSelf->GetActionState() )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			return false;
		}

		// 目标状态检查
		if( pPlayer->IsReadyState() || 
			( !pPlayer->IsStyleActionState() && !pPlayer->IsIdleState() ) || 
			EActionState_Normal != pPlayer->GetActionState() )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			return false;
		}
		
		// 友好度检查
		SocialMgr* pSocialMgr = (SocialMgr*)TObjRef<GameFrameMgr>()->GetFrame(_T("SocialMgr"));
		if( !P_VALID(pSocialMgr) || NULL == pSocialMgr->GetData() )
		{
			if( bShowErrMsg )
			{
				if(dwFriendVal>0)CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_FriendValNotEnough );
				else CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
			}
			return false;
		}

		const tagFriendInfo* pFriendInfo = pSocialMgr->GetData()->GetFriend( targetID );
		if(dwFriendVal>0)
		{
			if( !P_VALID(pFriendInfo) || pFriendInfo->dwFriVal < dwFriendVal )
			{
				if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_FriendValNotEnough );
				return false;
			}
		}
		else
		{
			if(!P_VALID(pFriendInfo))
			{
				if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_CanotActive );
				return false;
			}
		}			

		// 距离检查
		if( Vec3DistSq( pSelf->GetPos(), pTarget->GetPos() ) > MAX_MOTION_DIST * MAX_MOTION_DIST )
		{
			if( bShowErrMsg )	CombatSysUtil::Inst()->ShowPlayStyleActionErrMsg( E_Motion_DistanceNotEnough );
			return false;
		}
	}
	return true;
}

INT CombatActionChecker::GetCostByType( LocalPlayer* pSelf, const tagSkillData* pSkillData,ESkillCostType eSCT )
{
	INT nCost = pSkillData->pProto->nSkillCost[eSCT];

	if ( nCost > 100000 && ESCT_Rage != eSCT )
	{
		ERoleAttribute  nIndex = ERA_Null;
		float fTmpCost(0);
		fTmpCost = (float)nCost;

		if ( ESCT_HP == eSCT )
		{
			nIndex = ERA_MaxHP;
		}
		else if (ESCT_MP == eSCT)
		{
			nIndex = ERA_MaxMP;
		}
		else if (ESCT_Endurance == eSCT )
		{
			nIndex = ERA_MaxEndurance;
		}
		else if (ESCT_Valicity == eSCT )
		{
			nIndex = ERA_MaxVitality;
		}

		float fChangeRate = ((fTmpCost/100)-1000)/100;
		nCost = float(pSelf->GetAttribute(nIndex))*(fChangeRate); 
	}

	return nCost;
}