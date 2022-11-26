//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "hud.h"
#include "in_buttons.h"
#include "beamdraw.h"
#include "c_weapon__stubs.h"
//#include "ClientEffectPrecacheSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGravityGun )
//CLIENTEFFECT_MATERIAL( "sprites/physbeam" )
//CLIENTEFFECT_REGISTER_END()

class C_BeamQuadratic : public C_BaseEntity
{
public:
	C_BeamQuadratic();
	void			Update( C_BaseEntity *pOwner );

	void			Precache();

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return m_worldPosition; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual bool					ShouldDraw( void ) { return true; }
	virtual bool					IsTransparent( void ) { return true; }
	virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; }
	virtual int						DrawModel( int flags, const RenderableInstance_t &instance );

	// Returns the bounds relative to the origin (render bounds)
	/*virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		// bogus.  But it should draw if you can see the end point
		mins.Init(-32,-32,-32);
		maxs.Init(32,32,32);
	}*/

	virtual void GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( points[0], mins, maxs );
		AddPointToBounds( points[2], mins, maxs );
		AddPointToBounds( points[1], mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}


	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_worldPosition;
	Vector					points[3];
	int						m_active;
	int						m_glueTouching;
	int						m_viewModelIndex;
};


class C_WeaponGravityGun : public C_BaseCombatWeapon
{
	DECLARE_CLASS( C_WeaponGravityGun, C_BaseCombatWeapon );
public:
	C_WeaponGravityGun() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		if ( GetHud( 0 ).m_iKeyBits & IN_ATTACK )
		{
			switch ( keynum )
			{
			case MOUSE_WHEEL_UP:
				GetHud( 0 ).m_iKeyBits |= IN_WEAPON1;
				return 0;

			case MOUSE_WHEEL_DOWN:
				GetHud( 0 ).m_iKeyBits |= IN_WEAPON2;
				return 0;
			}
		}

		// Allow engine to process
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
	}

	void OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );
		m_beam.Update( this );
	}

private:
	C_WeaponGravityGun( const C_WeaponGravityGun & );

	C_BeamQuadratic	m_beam;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_physgun, C_WeaponGravityGun );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponGravityGun, DT_WeaponGravityGun, CWeaponGravityGun )
	RecvPropVector( RECVINFO_NAME(m_beam.m_targetPosition,m_targetPosition) ),
	RecvPropVector( RECVINFO_NAME(m_beam.m_worldPosition, m_worldPosition) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_active, m_active) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_glueTouching, m_glueTouching) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_viewModelIndex, m_viewModelIndex) ),
END_RECV_TABLE()


C_BeamQuadratic::C_BeamQuadratic()
{
	Assert( SetModel( "sprites/physbeam.vmt" ) );
	SetRenderMode( kRenderTransAdd );
	m_pOwner = NULL;
}

void C_BeamQuadratic::Update( C_BaseEntity *pOwner )
{
	m_pOwner = pOwner;
	if ( m_active )
	{
		if ( m_hRender == INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->AddRenderable( this, true, RENDERABLE_IS_TRANSLUCENT, RENDERABLE_MODEL_ENTITY );
		}
		else
		{
			ClientLeafSystem()->RenderableChanged( m_hRender );
		}
	}
	else if ( !m_active && m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_hRender );
	}
}

void C_BeamQuadratic::Precache()
{
	PrecacheMaterial( "sprites/physbeam" );
}

int	C_BeamQuadratic::DrawModel( int flags, const RenderableInstance_t &instance )
{
	QAngle tmpAngle;

	if ( !m_active )
		return 0;

	C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_viewModelIndex );
	if ( !pEnt )
		return 0;
	pEnt->GetAttachment( 1, points[0], tmpAngle );

	points[1] = 0.5 * (m_targetPosition + points[0]);
	
	// a little noise 11t & 13t should be somewhat non-periodic looking
	//points[1].z += 4*sin( gpGlobals->curtime*11 ) + 5*cos( gpGlobals->curtime*13 );
	points[2] = m_worldPosition;

	IMaterial *pMat = materials->FindMaterial( "sprites/physbeam", TEXTURE_GROUP_CLIENT_EFFECTS );
	Vector color;
	if ( m_glueTouching )
	{
		color.Init(1,0,0);
	}
	else
	{
		color.Init(1,1,1);
	}

	float scrollOffset = gpGlobals->curtime - (int)gpGlobals->curtime;
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->Bind( pMat );

	Draw_SetSpriteTexture( GetModel(), 0, GetRenderMode() );
	DrawBeamQuadratic( points[0], points[1], points[2], 13, color, scrollOffset );
	return 1;
}

