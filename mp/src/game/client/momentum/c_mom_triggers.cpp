#include "cbase.h"
#include "c_mom_triggers.h"
#include "model_types.h"
#include "util/mom_util.h"

#include "tier0/memdbgon.h"

const int SF_TELEPORT_PRESERVE_ANGLES = 0x20; // Preserve angles even when a local landmark is not specified

static ConVar mom_startzone_outline_enable("mom_startzone_outline_enable", "1",
                                           FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                           "Enable outline for start zone.");

static ConVar mom_endzone_outline_enable("mom_endzone_outline_enable", "1", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                         "Enable outline for end zone.");

static ConVar mom_startzone_color("mom_startzone_color", "00FF00FF", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                  "Color of the start zone.");

static ConVar mom_endzone_color("mom_endzone_color", "FF0000FF", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ARCHIVE,
                                "Color of the end zone.");

void C_BaseMomentumTrigger::UpdatePartitionListEntry()
{
	::partition->RemoveAndInsert( 
		PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS,  // remove
		PARTITION_CLIENT_TRIGGER_ENTITIES,  // add
		CollisionProp()->GetPartitionHandle() );
}

void TriggerProxy_Model(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	C_BaseMomentumTrigger *entity = (C_BaseMomentumTrigger *)pStruct;
	entity->SetModelName(pData->m_Value.m_pString);

	if (entity->SetModel(pData->m_Value.m_pString))
	{
		entity->SetSolid(SOLID_BSP);
		entity->AddSolidFlags(FSOLID_TRIGGER);
		entity->SetMoveType(MOVETYPE_NONE);

		Q_strncpy((char *)entity->m_iszModel.Get(), pData->m_Value.m_pString, MAX_TRIGGER_NAME);
		entity->PhysicsTouchTriggers();
	}
}

IMPLEMENT_CLIENTCLASS_DT(C_BaseMomentumTrigger, DT_BaseTrigger, CBaseTrigger)
	RecvPropString(RECVINFO(m_iszModel), NULL, TriggerProxy_Model),
	RecvPropString(RECVINFO(m_iszTarget)),
	RecvPropInt(RECVINFO(m_iSpawnFlags)),
END_RECV_TABLE();

bool C_BaseMomentumTrigger::PointIsWithin(const Vector &vecPoint)
{
    Ray_t ray;
    trace_t tr;
    ICollideable *pCollide = CollisionProp();
    ray.Init(vecPoint, vecPoint);
    enginetrace->ClipRayToCollideable(ray, MASK_ALL, pCollide, &tr);
    return (tr.startsolid);
}

bool C_BaseMomentumTrigger::PassesTriggerFilters(CBaseEntity *pOther)
{
    // First test spawn flag filters
    if (HasSpawnFlags(SF_TRIGGER_ALLOW_ALL) ||
        (HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
        (HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
        (HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
        (HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS))
    {
        if (pOther->GetFlags() & FL_NPC)
        {
            // TODO Make npc stuff
            /*CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

            if (HasSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS))
            {
                if (!pNPC || !pNPC->IsPlayerAlly())
                {
                    return false;
                }
            }

            if (HasSpawnFlags(SF_TRIGGER_ONLY_NPCS_IN_VEHICLES))
            {
                if (!pNPC || !pNPC->IsInAVehicle())
                    return false;
            }*/
            Msg("[Client] PassesTriggerFilters is NPC");
            return false;
        }

        bool bOtherIsPlayer = pOther->IsPlayer();

        if (bOtherIsPlayer)
        {
            CBasePlayer *pPlayer = (CBasePlayer *)pOther;
            if (!pPlayer->IsAlive())
                return false;

            // TODO Make vehicle stuff
            if (HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES))
            {
                if (!pPlayer->IsInAVehicle())
                {
                    Msg("[Client] PassesTriggerFilters is not inside vehicle");
                    return false;
                }

                // Make sure we're also not exiting the vehicle at the moment
                /*IServerVehicle *pVehicleServer = pPlayer->GetVehicle();
                if (pVehicleServer == NULL)
                    return false;

                if (pVehicleServer->IsPassengerExiting())
                    return false;*/
            }

            if (HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES))
            {
                if (pPlayer->IsInAVehicle())
                {
                    Msg("[Client] PassesTriggerFilters is inside vehicle");
                    return false;
                }
            }

            // TODO Make bots stuff
            /*if (HasSpawnFlags(SF_TRIGGER_DISALLOW_BOTS))
            {
                if (pPlayer->IsFakeClient())
                    return false;
            }*/
        }

        return true;

        // TODO Make filter stuff
        // CBaseFilter *pFilter = m_hFilter.Get();
        // return (!pFilter) ? true : pFilter->PassesFilter(this, pOther);
    }
    Msg("[Client] Wat: %08X, %08X, %08X\n", (pOther->GetFlags() & FL_CLIENT) == FL_CLIENT,
        HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS), m_iSpawnFlags);
    return false;
}

LINK_ENTITY_TO_CLASS(trigger_momentum_timer_start, C_TriggerTimerStart);

IMPLEMENT_CLIENTCLASS_DT(C_TriggerTimerStart, DT_TriggerTimerStart, CTriggerTimerStart)
END_RECV_TABLE();

LINK_ENTITY_TO_CLASS(trigger_momentum_timer_stop, C_TriggerTimerStop);

IMPLEMENT_CLIENTCLASS_DT(C_TriggerTimerStop, DT_TriggerTimerStop, CTriggerTimerStop)
END_RECV_TABLE();

static class CTriggerOutlineRenderer : public IBrushRenderer, public CAutoGameSystem
{
  public:
    CTriggerOutlineRenderer()
    {
        m_pVertices = NULL;
        m_verticeCount = 0;
    }

    bool RenderBrushModelSurface(IClientEntity *pBaseEntity, IBrushSurface *pBrushSurface) OVERRIDE
    {
        const int vertices = pBrushSurface->GetVertexCount();
        if (vertices > m_verticeCount)
        {
            m_verticeCount = vertices;
            if (m_pVertices == NULL)
                m_pVertices =
                    static_cast<BrushVertex_t *>(MemAlloc_AllocAligned(sizeof(BrushVertex_t) * m_verticeCount, 64));
            else
                m_pVertices = static_cast<BrushVertex_t *>(
                    MemAlloc_ReallocAligned(m_pVertices, sizeof(BrushVertex_t) * m_verticeCount, 64));
        }
        pBrushSurface->GetVertexData(m_pVertices);
        CMatRenderContextPtr pRenderContext(materials);

        CMeshBuilder builder;
        builder.Begin(pRenderContext->GetDynamicMesh(true, 0, 0, outlineMaterial), MATERIAL_LINE_LOOP, vertices);
        for (int i = 0; i < vertices; i++)
        {
            const BrushVertex_t &vertex = m_pVertices[i];
            builder.Position3fv(vertex.m_Pos.Base());
            builder.Normal3fv(vertex.m_Normal.Base());
            builder.Color4ub(outlineColor.r(), outlineColor.g(), outlineColor.b(), outlineColor.a());
            builder.AdvanceVertex();
        }

        builder.End(false, true);

        return false;
    }

    bool Init() OVERRIDE
    {
        KeyValues *pVMTKeyValues = new KeyValues("unlitgeneric");
        pVMTKeyValues->SetString("$vertexcolor", "1");
        pVMTKeyValues->SetString("$vertexalpha", "1");
        pVMTKeyValues->SetString("$additive", "1");
        pVMTKeyValues->SetString("$ignorez", "0"); // Change this to 1 to see it through walls
        pVMTKeyValues->SetString("$halflambert", "1");
        pVMTKeyValues->SetString("$selfillum", "1");
        pVMTKeyValues->SetString("$nofog", "1");
        pVMTKeyValues->SetString("$nocull", "1");
        pVMTKeyValues->SetString("$model", "1");
        outlineMaterial.Init("__utilOutlineColor", pVMTKeyValues);
        outlineMaterial->Refresh();

        return true;
    }

    void Shutdown() OVERRIDE
    {
        MemAlloc_FreeAligned(m_pVertices);
        m_pVertices = NULL;
        outlineMaterial.Shutdown();
    }

    Color outlineColor;

  private:
    CMaterialReference outlineMaterial;
    BrushVertex_t *m_pVertices;
    int m_verticeCount;
} outlineRenderer;

bool C_TriggerTimerStart::ShouldDraw() { return true; }

int C_TriggerTimerStart::DrawModel(int flags)
{
    if (mom_startzone_outline_enable.GetBool() && (flags & STUDIO_RENDER) != 0 &&
        (flags & (STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SHADOWDEPTHTEXTURE)) == 0)
    {
        if (g_pMomentumUtil->GetColorFromHex(mom_startzone_color.GetString(), outlineRenderer.outlineColor))
        {
            render->InstallBrushSurfaceRenderer(&outlineRenderer);
            BaseClass::DrawModel(STUDIO_RENDER);
            render->InstallBrushSurfaceRenderer(nullptr);
            if (IsEffectActive(EF_NODRAW))
                return 1;
        }
    }

    return BaseClass::DrawModel(flags);
}

bool C_TriggerTimerStop::ShouldDraw() { return true; }

int C_TriggerTimerStop::DrawModel(int flags)
{
    if (mom_startzone_outline_enable.GetBool() && (flags & STUDIO_RENDER) != 0 &&
        (flags & (STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SHADOWDEPTHTEXTURE)) == 0)
    {
        if (g_pMomentumUtil->GetColorFromHex(mom_endzone_color.GetString(), outlineRenderer.outlineColor))
        {
            render->InstallBrushSurfaceRenderer(&outlineRenderer);
            BaseClass::DrawModel(STUDIO_RENDER);
            render->InstallBrushSurfaceRenderer(nullptr);
            if (IsEffectActive(EF_NODRAW))
                return 1;
        }
    }

    return BaseClass::DrawModel(flags);
}

LINK_ENTITY_TO_CLASS(trigger_momentum_slide, C_TriggerSlide);

IMPLEMENT_CLIENTCLASS_DT(C_TriggerSlide, DT_TriggerSlide, CTriggerSlide)
	RecvPropBool(RECVINFO(m_bStuckOnGround)),
	RecvPropBool(RECVINFO(m_bAllowingJump)),
	RecvPropBool(RECVINFO(m_bDisableGravity)),
	RecvPropBool(RECVINFO(m_bFixUpsideSlope)),
END_RECV_TABLE();

LINK_ENTITY_TO_CLASS(trigger_teleport, C_TriggerTeleport);

IMPLEMENT_CLIENTCLASS_DT(C_TriggerTeleport, DT_TriggerTeleport, CTriggerTeleport)
	RecvPropString(RECVINFO(m_iszLandmark)),
END_RECV_TABLE();

CBaseEntity *FindEntityByClassname(CBaseEntity *pEnt, const char *szName)
{
	CBaseEntity *pNext = cl_entitylist->NextBaseEntity(pEnt);

	while (pNext != nullptr)
	{
		if (Q_strcmp(pNext->GetName(), szName) == 0)
		{
			return pNext;
		}

		pNext = cl_entitylist->NextBaseEntity(pNext);
	}
	return nullptr;
}

void C_TriggerTeleport::StartTouch(CBaseEntity *pOther)
{
	CBaseEntity *pentTarget = NULL;
	CBaseEntity *pentLandmark = NULL;

	if (!PassesTriggerFilters(pOther))
	{
		return;
	}

	// The activator and caller are the same
	pentTarget = FindEntityByClassname(pentTarget, m_iszTarget.Get());

	if (!pentTarget)
	{
		Msg("[Client] Could not find target!\n");
		return;
	}

	Vector vecLandmarkOffset(0, 0, 0);
	if (m_iszLandmark.Get()[0] != 0)
	{
		// The activator and caller are the same
		pentLandmark = FindEntityByClassname(pentLandmark, m_iszLandmark.Get());

		if (pentLandmark)
		{
			vecLandmarkOffset = pOther->GetAbsOrigin() - pentLandmark->GetAbsOrigin();
		}
	}

	Vector tmp = pentTarget->GetAbsOrigin();
	QAngle tmp_angle = pentTarget->GetAbsAngles();

	if (!pentLandmark && pOther->IsPlayer())
	{
		// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
		tmp.z -= pOther->WorldAlignMins().z;
	}

	tmp += vecLandmarkOffset;
	
	pOther->SetGroundEntity(NULL);

	if (!pentLandmark && !HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES))
	{
		pOther->m_angNetworkAngles = tmp_angle;
		pOther->SetLocalAngles(tmp_angle);
	}

	pOther->m_vecNetworkOrigin = tmp;
	pOther->SetLocalOrigin(tmp);
	pOther->SetAbsOrigin(tmp);

	C_BasePlayer* player = C_BasePlayer::GetLocalPlayer();
	if (player == pOther)
	{
		// We need to do it this way to set viewangles at the same frame as the new orign is viewed to the screen
		if (prediction->GetIsFirstTimePredicted() && !pentLandmark && !HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES))
		{
			player->m_bFixViewAngle = true;
			player->m_vecFixedViewAngles = tmp_angle;
		}

		if (!pentLandmark && !HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES))
		{
			prediction->SetLocalViewAngles(tmp_angle);
		}

		prediction->SetViewOrigin(tmp);
	}
}

LINK_ENTITY_TO_CLASS(info_teleport_destination, C_PointEntity);

IMPLEMENT_CLIENTCLASS_DT(C_PointEntity, DT_PointEntity, CPointEntity)
	RecvPropString(RECVINFO(m_iszName)),
END_RECV_TABLE();

void C_PointEntity::Spawn()
{
	SetSolid(SOLID_NONE);
}

LINK_ENTITY_TO_CLASS(trigger_push, C_TriggerPush);

void TriggerPushProxy_PushDir(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	C_TriggerPush *entity = (C_TriggerPush *)pStruct;

	*(float*)&entity->m_vecPushDir.Get().x = pData->m_Value.m_Vector[0];
	*(float*)&entity->m_vecPushDir.Get().y = pData->m_Value.m_Vector[1];
	*(float*)&entity->m_vecPushDir.Get().z = pData->m_Value.m_Vector[2];
	
	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle(entity->m_vecPushDir.Get().x, entity->m_vecPushDir.Get().y, entity->m_vecPushDir.Get().z);
	AngleVectors(angPushDir, &vecAbsDir);

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, entity->EntityToWorldTransform(), (Vector)entity->m_vecPushDir.Get() );

	if (entity->m_flSpeed == 0)
	{
		entity->m_flSpeed = 100;
	}
}

IMPLEMENT_CLIENTCLASS_DT(C_TriggerPush, DT_TriggerPush, CTriggerPush)
	RecvPropFloat(RECVINFO(m_flAlternateTicksFix)),
	RecvPropFloat(RECVINFO(m_flPushSpeed)),
	RecvPropVector(RECVINFO(m_vecPushDir), 0, TriggerPushProxy_PushDir),
END_RECV_TABLE();

void C_TriggerPush::Spawn(void)
{
	m_iUserID = INVALID_USER_ID;
	m_nTickBasePush = -1;
	BaseClass::Spawn();
}

void C_TriggerPush::Activate(void)
{
	BaseClass::Activate();
}

void C_TriggerPush::Touch(CBaseEntity * pOther)
{
	if ( !pOther->IsSolid() || (pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE ) )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	// FIXME: If something is hierarchically attached, should we try to push the parent?
	if (pOther->GetMoveParent())
		return;

	if (!pOther->IsPlayer())
		return;

	C_BasePlayer* player = (C_BasePlayer*)pOther;

	// Transform the push dir into global space
	Vector vecAbsDir;
	VectorRotate( m_vecPushDir.Get(), EntityToWorldTransform(), vecAbsDir );

	// Instant trigger, just transfer velocity and remove
	if (HasSpawnFlags(SF_TRIG_PUSH_ONCE) &&
		((player->GetTickBase() == m_nTickBasePush && player->GetUserID() == m_iUserID) ||
		(m_nTickBasePush == -1 && m_iUserID == INVALID_USER_ID)))
	{
		pOther->ApplyAbsVelocityImpulse( m_flPushSpeed * vecAbsDir );

		if ( vecAbsDir.z > 0 )
		{
			pOther->SetGroundEntity( NULL );
		}

		m_iUserID = player->GetUserID();
		m_nTickBasePush = player->GetTickBase();
		return;
	}

	switch( pOther->GetMoveType() )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
		break;

	case MOVETYPE_VPHYSICS:
		{
			IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
			if ( pPhys )
			{
				// UNDONE: Assume the velocity is for a 100kg object, scale with mass
				pPhys->ApplyForceCenter( m_flPushSpeed * vecAbsDir * 100.0f * gpGlobals->frametime );
				return;
			}
		}
		break;

	default:
		{
			Vector vecPush = (m_flPushSpeed * vecAbsDir);
			if (pOther->GetFlags() & FL_BASEVELOCITY)
			{
				vecPush = vecPush + pOther->GetBaseVelocity();
			}
			if (vecPush.z > 0 && (pOther->GetFlags() & FL_ONGROUND))
			{
				pOther->SetGroundEntity(NULL);
				Vector origin = pOther->GetAbsOrigin();
				origin.z += 1.0f;
				pOther->SetAbsOrigin(origin);
			}

			pOther->SetBaseVelocity(vecPush);
			pOther->AddFlag(FL_BASEVELOCITY);
		}
		break;
	}
}
