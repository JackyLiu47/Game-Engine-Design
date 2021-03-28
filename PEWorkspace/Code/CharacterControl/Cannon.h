#ifndef _CHARACTER_CONTROL_CANNON_
#define _CHARACTER_CONTROL_CANNON_

#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Math/Matrix4x4.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "Events/Events.h"

#define MAX_NUMS_BALL 25

namespace CharacterControl{
namespace Events{
struct Event_CREATE_CANNON : public PE::Events::Event
{
	PE_DECLARE_CLASS(Event_CREATE_CANNON);

	// override SetLuaFunctions() since we are adding custom Lua interface
	static void SetLuaFunctions(PE::Components::LuaEnvironment *pLuaEnv, lua_State *luaVM);

	// Lua interface prefixed with l_
	static int l_Construct(lua_State* luaVM);

	Matrix4x4 m_base;

	PEUUID m_peuuid; // unique object id
};
}
namespace Components {

struct Cannon : public PE::Components::Component
{
	PE_DECLARE_CLASS(Cannon);

	Cannon( PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, const Events::Event_CREATE_CANNON *pEvt);

	virtual void addDefaultComponents();

	PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
	virtual void do_UPDATE(PE::Events::Event* pEvt);

	void SpawnBall();

	Matrix4x4 m_base;
	Array<PE::Handle> m_balls;
	float m_timer;
};
}; // namespace Components
}; // namespace CharacterControl
#endif

