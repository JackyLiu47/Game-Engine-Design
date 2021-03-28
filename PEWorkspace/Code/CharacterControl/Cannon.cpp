#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Lua/EventGlue/EventDataCreators.h"

#include "PrimeEngine/Physics/PhysicsComponent.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "Cannon.h"
#include "Ball.h"
#include <stdlib.h>

using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;
using namespace PE::Events;

namespace CharacterControl{
namespace Events{

PE_IMPLEMENT_CLASS1(Event_CREATE_CANNON, PE::Events::Event);

void Event_CREATE_CANNON::SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM)
{
	static const struct luaL_Reg l_Event_CREATE_CANNON[] = {
		{"Construct", l_Construct},
		{NULL, NULL} // sentinel
	};

	// register the functions in current lua table which is the table for Event_CreateSoldierNPC
	luaL_register(luaVM, 0, l_Event_CREATE_CANNON);
}

int Event_CREATE_CANNON::l_Construct(lua_State* luaVM)
{
    PE::Handle h("EVENT", sizeof(Event_CREATE_CANNON));
	Event_CREATE_CANNON *pEvt = new(h) Event_CREATE_CANNON;

	// get arguments from stack
	int numArgs, numArgsConst;
	numArgs = numArgsConst = 13;

	float positionFactor = 1.0f / 100.0f;
	Vector3 pos, u, v, n;
	pos.m_x = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_y = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;
	pos.m_z = (float)lua_tonumber(luaVM, -numArgs--) * positionFactor;

	u.m_x = (float)lua_tonumber(luaVM, -numArgs--); u.m_y = (float)lua_tonumber(luaVM, -numArgs--); u.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	v.m_x = (float)lua_tonumber(luaVM, -numArgs--); v.m_y = (float)lua_tonumber(luaVM, -numArgs--); v.m_z = (float)lua_tonumber(luaVM, -numArgs--);
	n.m_x = (float)lua_tonumber(luaVM, -numArgs--); n.m_y = (float)lua_tonumber(luaVM, -numArgs--); n.m_z = (float)lua_tonumber(luaVM, -numArgs--);

	pEvt->m_peuuid = LuaGlue::readPEUUID(luaVM, -numArgs--);
	
	lua_pop(luaVM, numArgsConst); //Second arg is a count of how many to pop

	pEvt->m_base.loadIdentity();
	pEvt->m_base.setPos(pos);
	pEvt->m_base.setU(u);
	pEvt->m_base.setV(v);
	pEvt->m_base.setN(n);

	LuaGlue::pushTableBuiltFromHandle(luaVM, h); 

	return 1;
}
};
namespace Components {

PE_IMPLEMENT_CLASS1(Cannon, Component);

// create waypoint form creation event
Cannon::Cannon(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_CANNON *pEvt)
: Component(context, arena, hMyself)
, m_balls(context, arena, MAX_NUMS_BALL)
{
	m_base = pEvt->m_base;
	m_timer = 0;
}

void Cannon::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, Cannon::do_UPDATE);
	SpawnBall();
}

void Cannon::do_UPDATE(PE::Events::Event* pEvt)
{
	Event_UPDATE* pTrueEvent = (Event_UPDATE*)(pEvt);

	if (m_timer > 2) {
		// spawn a ball
		SpawnBall();
		m_timer = 0;
	}
	else {
		m_timer += pTrueEvent->m_frameTime;
	}
}

void Cannon::SpawnBall() {
	// find a slot
	int slot = -1;
	bool full = m_balls.m_size == m_balls.m_capacity;
	if (full) {
		/*for (int i = 0; i < m_balls.m_size; ++i) {
			if (!m_balls[i].isValid()) {
				slot = i;
				break;
			}
		}*/
		return; // could not spawn a ball
	}

	PE::Handle hBall("BALL", sizeof(Ball));
	Ball* pBall = new(hBall) Ball(*m_pContext, m_arena, hBall, m_base);
	pBall->addDefaultComponents();
	PhysicsComponent* pPhysics = pBall->m_hPhysics.getObject<PhysicsComponent>();

	//Vector3 velocity = Vector3(rand() % 40 - 20, rand() % 10 + 5, rand() % 40 - 20);
	Vector3 velocity = Vector3(6, 8.5, 6);
	Vector3 aVelocity = Vector3(0, 4, 0);
	//Vector3 aVelocity = Vector3(rand() % 4 - 2, rand() % 3 + 2, rand() % 4 - 2);
	aVelocity.normalize();
	aVelocity *= max(rand() % 7, 3);
	pPhysics->setVelocity(velocity);
	pPhysics->setAVelocity(aVelocity);
	pPhysics->m_mass = 0.5;
	pPhysics->m_bounce = 0.7;
	pPhysics->m_angularAttenuation = 0.2;

	if (!full) {
		m_balls.add(hBall);
	}
	/*else {
		m_balls[slot] = hBall;
	}*/
	PEINFO("Spawning a ball\n");
}

}; // namespace Components
}; // namespace CharacterControl
