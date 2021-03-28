#ifndef __pe_physicsmanager_h__
#define __pe_physicsmanager_h__

#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"

// Sibling/Children includes


#define GRAVITY 9.8

namespace PE {
	namespace Components {

		struct PhysicsManager : public Component
		{
			PE_DECLARE_CLASS(PhysicsManager);

			// Constructor -------------------------------------------------------------
			PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);

			virtual ~PhysicsManager() {}

			virtual void addDefaultComponents();

			void registerEvents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PHYSICS_START)
			virtual void do_PHYSICS_START(PE::Events::Event* pEvt);
			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event* pEvt);
			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PHYSICS_END)
			virtual void do_PHYSICS_END(PE::Events::Event* pEvt);
			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_RENDER_needsRC)
			virtual void do_PRE_RENDER_needsRC(PE::Events::Event* pEvt);

			static void Construct(PE::GameContext& context, PE::MemoryArena arena);
			static void SetInstance(Handle h);
			static PhysicsManager* Instance();
			static Handle s_hInstance;

			// need this to maintain m_instances
			virtual void addComponent(Handle hComponent, int* pAllowedEvents = NULL);
			//virtual void removeComponent(int index);

			//void addPhysicsHandle(Handle h);
			float m_dt;
			bool m_debug = true;

			Array<Handle> m_pComponents;

			void naiveUpdate();
			void checkCollision();
			void syncSceneNodes();
		};

	}; // namespace Components
}; // namespace PE
#endif


