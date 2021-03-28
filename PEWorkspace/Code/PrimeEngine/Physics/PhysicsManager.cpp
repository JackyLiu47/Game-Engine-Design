#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PhysicsManager.h"
// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

// Sibling/Children includes
#include "PhysicsComponent.h"
#include "../Lua/LuaEnvironment.h"

namespace PE {
	namespace Components {

		PE_IMPLEMENT_CLASS1(PhysicsManager, Component);

		Handle PhysicsManager::s_hInstance;

		PhysicsManager::PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself)
			: Component(context, arena, hMyself)
			, m_pComponents(context, arena, 64)
		{}


		void PhysicsManager::addComponent(Handle hComponent, int* pAllowedEvents /* = NULL */)
		{
			if (hComponent.getObject<Component>()->isInstanceOf<PhysicsComponent>())
			{
				m_pComponents.add(hComponent);
			}

			Component::addComponent(hComponent, pAllowedEvents);
		}

		void PhysicsManager::naiveUpdate()
		{
			int size = m_pComponents.m_size;
			for (int i = 0; i < size; ++i)
			{
				PhysicsComponent* comp = m_pComponents[i].getObject<PhysicsComponent>();
				comp->m_posCorrection = comp->m_base.getPos();
				comp->update(m_dt);
				comp->m_noMove = Vector3();
				comp->m_rolledBack = false;
				//comp->calculateOBB();
			}
		}

		void PhysicsManager::checkCollision()
		{
			int size = m_pComponents.m_size;
			for (int i = 0; i < size - 1; ++i)
			{
				if (!m_pComponents[i].isValid()) continue;
				PhysicsComponent* comp1 = m_pComponents[i].getObject<PhysicsComponent>();
				if (!comp1->m_root)
					continue;
				for (int j = i + 1; j < size; ++j)
				{
					if (!m_pComponents[j].isValid()) continue;
					PhysicsComponent* comp2 = m_pComponents[j].getObject<PhysicsComponent>();

					// ignore if the same mesh instance
					if (!comp2->m_root)
						continue;

					// ignore static cases
					if (!comp1->isMoving() && !comp2->isMoving())
						continue;

					// check if i and j has collision
					if (comp1->isColliding(comp2))
					{
						// rollback calculation for both
						if (!comp1->m_static)
						{
							comp1->correctMovements();
						}
							
						if (!comp2->m_static)
						{
							comp2->correctMovements();
						}
					}
				}
			}
			// re-calculate position
			for (int j = 0; j < size; ++j) {
				PhysicsComponent* comp = m_pComponents[j].getObject<PhysicsComponent>();
				if (comp->m_rolledBack)
					/*comp->updatePosition(m_dt * comp->m_timeLeft);*/
					comp->updatePosition(m_dt);
			}
		}

		void PhysicsManager::syncSceneNodes()
		{
			int size = m_pComponents.m_size;
			for (int i = 0; i < size; ++i)
			{
				PhysicsComponent* comp = m_pComponents[i].getObject<PhysicsComponent>();
				comp->syncSceneNode();
			}
		}

		/*void PhysicsManager::removeComponent(int index)
		{
			Handle hComponent = m_pComponents[index];
			if (hComponent.getObject<Component>()->isInstanceOf<PhysicsComponent>())
			{
				m_pComponents.remove(m_pComponents.indexOf(hComponent));
			}

			Component::removeComponent(index);
		}*/


		void PhysicsManager::addDefaultComponents()
		{
			Component::addDefaultComponents();
			registerEvents();
		}

		void PhysicsManager::registerEvents()
		{
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PHYSICS_START, PhysicsManager::do_PHYSICS_START);
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, PhysicsManager::do_UPDATE);
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PHYSICS_END, PhysicsManager::do_PHYSICS_END);
			PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PRE_RENDER_needsRC, PhysicsManager::do_PRE_RENDER_needsRC);
			
		}

		void PhysicsManager::do_PRE_RENDER_needsRC(PE::Events::Event* pEvt) {
			if (m_debug) {
				int size = m_pComponents.m_size;
				for (int i = 0; i < size; ++i)
				{
					PhysicsComponent* comp = m_pComponents[i].getObject<PhysicsComponent>();
					comp->drawVolumes();
				}
			}
		}

		void PhysicsManager::do_PHYSICS_START(PE::Events::Event* pEvt)
		{
			//PE::Events::Event_PHYSICS_START* pRealEvt = (PE::Events::Event_PHYSICS_START*)pEvt;
			naiveUpdate();
		}

		void PhysicsManager::do_UPDATE(PE::Events::Event* pEvt)
		{
			PE::Events::Event_UPDATE* pRealEvt = (PE::Events::Event_UPDATE*)pEvt;
			m_dt = pRealEvt->m_frameTime;
		}

		void PhysicsManager::do_PHYSICS_END(PE::Events::Event* pEvt)
		{
			//PE::Events::Event_PHYSICS_END* pRealEvt = (PE::Events::Event_PHYSICS_END*)pEvt;
			checkCollision();
			syncSceneNodes();
		}

		void PhysicsManager::Construct(PE::GameContext& context, PE::MemoryArena arena)
		{
			Handle h("PHYSICS_MANAGER", sizeof(PhysicsManager));
			PhysicsManager* pPhysicsManager = new(h) PhysicsManager(context, arena, h);
			pPhysicsManager->addDefaultComponents();
			SetInstance(h);
		}

		void PhysicsManager::SetInstance(Handle h)
		{
			s_hInstance = h;
		}

		PhysicsManager* PhysicsManager::Instance()
		{
			return s_hInstance.getObject<PhysicsManager>();
		}

	}; // namespace Components
}; // namespace PE
