#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Physics/PhysicsComponent.h"

#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPC.h"
using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl {

	// Events sent by behavior state machine (or other high level state machines)
	// these are events that specify where a soldier should move
	namespace Events {

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_MOVE_TO, Event);

		SoldierNPCMovementSM_Event_MOVE_TO::SoldierNPCMovementSM_Event_MOVE_TO(Vector3 targetPos /* = Vector3 */)
			: m_targetPosition(targetPos)
		{ }

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_STOP, Event);

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_TARGET_REACHED, Event);
	}

	namespace Components {

		PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM, Component);


		SoldierNPCMovementSM::SoldierNPCMovementSM(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself)
			: Component(context, arena, hMyself)
			, m_state(STANDING)
		{}

		SceneNode* SoldierNPCMovementSM::getParentsSceneNode()
		{
			PE::Handle hParent = getFirstParentByType<Component>();
			if (hParent.isValid())
			{
				// see if parent has scene node component
				return hParent.getObject<Component>()->getFirstComponent<SceneNode>();

			}
			return NULL;
		}

		void SoldierNPCMovementSM::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_MOVE_TO, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO);
			PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_STOP, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP);

			PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCMovementSM::do_UPDATE);
		}

		void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO(PE::Events::Event* pEvt)
		{
			SoldierNPCMovementSM_Event_MOVE_TO* pRealEvt = (SoldierNPCMovementSM_Event_MOVE_TO*)(pEvt);

			// change state of this state machine
			m_state = WALKING_TO_TARGET;
			m_targetPostion = pRealEvt->m_targetPosition;

			// make sure the animations are playing

			PE::Handle h("SoldierNPCAnimSM_Event_WALK", sizeof(SoldierNPCAnimSM_Event_WALK));
			Events::SoldierNPCAnimSM_Event_WALK* pOutEvt = new(h) SoldierNPCAnimSM_Event_WALK();

			SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
			pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

			// release memory now that event is processed
			h.release();
		}

		void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP(PE::Events::Event* pEvt)
		{
			Events::SoldierNPCAnimSM_Event_STOP Evt;

			SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
			pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&Evt);
		}

		void SoldierNPCMovementSM::do_UPDATE(PE::Events::Event* pEvt)
		{
			if (m_state == WALKING_TO_TARGET)
			{
				// see if parent has scene node component
				SceneNode* pSN = getParentsSceneNode();
				if (pSN)
				{
					PE::Handle hPC = pSN->m_hPhysics;
					PhysicsComponent* pPC = hPC.getObject<PhysicsComponent>();

					bool reached = pPC->isWithin(m_targetPostion, 0.1f);
					if (!reached)
					{
						pPC->goTo(m_targetPostion, 2.f, 0.1f);
					}

					if (reached)
					{
						m_state = STANDING;
						pPC->setVelocity(Vector3());
						// target has been reached. need to notify all same level state machines (components of parent)
						{
							PE::Handle h("SoldierNPCMovementSM_Event_TARGET_REACHED", sizeof(SoldierNPCMovementSM_Event_TARGET_REACHED));
							Events::SoldierNPCMovementSM_Event_TARGET_REACHED* pOutEvt = new(h) SoldierNPCMovementSM_Event_TARGET_REACHED();

							PE::Handle hParent = getFirstParentByType<Component>();
							if (hParent.isValid())
							{
								hParent.getObject<Component>()->handleEvent(pOutEvt);
							}

							// release memory now that event is processed
							h.release();
						}

						if (m_state == STANDING)
						{
							// no one has modified our state based on TARGET_REACHED callback
							// this means we are not going anywhere right now
							// so can send event to animation state machine to stop
							{
								Events::SoldierNPCAnimSM_Event_STOP evt;

								SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
								pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&evt);
							}
						}
					}
				}
			}
		}

	}
}




