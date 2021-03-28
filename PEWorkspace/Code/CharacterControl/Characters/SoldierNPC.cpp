#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/SkeletonInstance.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Physics/PhysicsComponent.h"

#include "SoldierNPC.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCBehaviorSM.h"



using namespace PE;
using namespace PE::Components;
using namespace CharacterControl::Events;

namespace CharacterControl {
	namespace Components {

		PE_IMPLEMENT_CLASS1(SoldierNPC, Component);

		SoldierNPC::SoldierNPC(PE::GameContext& context, PE::MemoryArena arena, PE::Handle hMyself, Event_CreateSoldierNPC* pEvt) : Component(context, arena, hMyself)
		{

			// hierarchy of soldier and replated components and variables (note variables are just variables, they are not passed events to)
			// scene
			// +-components
			//   +-soldier scene node
			//   | +-components
			//   |   +-soldier skin
			//   |     +-components
			//   |       +-soldier animation state machine
			//   |       +-soldier weapon skin scene node
			//   |         +-components
			//   |           +-weapon mesh

			// game objects
			// +-components
			//   +-soldier npc
			//     +-variables
			//     | +-m_hMySN = soldier scene node
			//     | +-m_hMySkin = skin
			//     | +-m_hMyGunSN = soldier weapon skin scene node
			//     | +-m_hMyGunMesh = weapon mesh
			//     +-components
			//       +-soldier scene node (restricted to no events. this is for state machines to be able to locate the scene node)
			//       +-movement state machine
			//       +-behavior state machine


			// need to acquire redner context for this code to execute thread-safe
			m_pContext->getGPUScreen()->AcquireRenderContextOwnership(pEvt->m_threadOwnershipMask);

			PE::Handle hSN("SCENE_NODE", sizeof(SceneNode));
			SceneNode* pMainSN = new(hSN) SceneNode(*m_pContext, m_arena, hSN);
			pMainSN->addDefaultComponents();

			pMainSN->m_base.setPos(pEvt->m_pos);
			pMainSN->m_base.setU(pEvt->m_u);
			pMainSN->m_base.setV(pEvt->m_v);
			pMainSN->m_base.setN(pEvt->m_n);


			RootSceneNode::Instance()->addComponent(hSN);

			// add the scene node as component of soldier without any handlers. this is just data driven way to locate scnenode for soldier's components
			{
				static int allowedEvts[] = { 0 };
				addComponent(hSN, &allowedEvts[0]);
			}

			int numskins = 1; // 8
			for (int iSkin = 0; iSkin < numskins; ++iSkin)
			{
				float z = (iSkin / 4) * 1.5f;
				float x = (iSkin % 4) * 1.5f;
				PE::Handle hsSN("SCENE_NODE", sizeof(SceneNode));
				SceneNode* pSN = new(hsSN) SceneNode(*m_pContext, m_arena, hsSN);
				pSN->addDefaultComponents();

				pSN->m_base.setPos(Vector3(x, 0, z));

				// rotation scene node to rotate soldier properly, since soldier from Maya is facing wrong direction
				PE::Handle hRotateSN("SCENE_NODE", sizeof(SceneNode));
				SceneNode* pRotateSN = new(hRotateSN) SceneNode(*m_pContext, m_arena, hRotateSN);
				pRotateSN->addDefaultComponents();

				pSN->addComponent(hRotateSN);

				pRotateSN->m_base.turnLeft(3.1415);

				PE::Handle hSoldierAnimSM("SoldierNPCAnimationSM", sizeof(SoldierNPCAnimationSM));
				SoldierNPCAnimationSM* pSoldierAnimSM = new(hSoldierAnimSM) SoldierNPCAnimationSM(*m_pContext, m_arena, hSoldierAnimSM);
				pSoldierAnimSM->addDefaultComponents();

				pSoldierAnimSM->m_debugAnimIdOffset = 0;// rand() % 3;

				PE::Handle hSkeletonInstance("SkeletonInstance", sizeof(SkeletonInstance));
				SkeletonInstance* pSkelInst = new(hSkeletonInstance) SkeletonInstance(*m_pContext, m_arena, hSkeletonInstance,
					hSoldierAnimSM);
				pSkelInst->addDefaultComponents();

				pSkelInst->initFromFiles("soldier_Soldier_Skeleton.skela", "Soldier", pEvt->m_threadOwnershipMask);

				pSkelInst->setAnimSet("soldier_Soldier_Skeleton.animseta", "Soldier");

				PE::Handle hMeshInstance("MeshInstance", sizeof(MeshInstance));
				MeshInstance* pMeshInstance = new(hMeshInstance) MeshInstance(*m_pContext, m_arena, hMeshInstance);
				pMeshInstance->addDefaultComponents();

				pMeshInstance->initFromFile(pEvt->m_meshFilename, pEvt->m_package, pEvt->m_threadOwnershipMask);

				pSkelInst->addComponent(hMeshInstance);

				float boxScale = 1.5;
				Mesh* pMesh = pMeshInstance->m_hAsset.getObject<Mesh>();
				Vector3 maxAABB = pMesh->m_aabb[1];
				Vector3 minAABB = pMesh->m_aabb[0];
				bool useX = abs(maxAABB.m_x - minAABB.m_x) < abs(maxAABB.m_z - minAABB.m_z);
				float radiusBox = useX ? abs(maxAABB.m_x - minAABB.m_x) : abs(maxAABB.m_z - minAABB.m_z);
				
				float radius = radiusBox * 0.5 * boxScale;
				float xmean = (maxAABB.m_x + minAABB.m_x) * 0.5;
				float zmean = (maxAABB.m_z + minAABB.m_z) * 0.5;
				float ymaxBox = maxAABB.m_y - radius;
				float yminBox = minAABB.m_y + radius;
				Vector3 centerTop = Vector3(xmean, ymaxBox, zmean);
				Vector3 centerBot = Vector3(xmean, yminBox, zmean);
				Array<Vector3> boxAABB = Array<Vector3>(context, arena, 2);
				float minVal = (useX ? minAABB.m_x : minAABB.m_z) * boxScale;
				float maxVal = (useX ? maxAABB.m_x : maxAABB.m_z) * boxScale;
				boxAABB.add(Vector3(minVal, yminBox, minVal));
				boxAABB.add(Vector3(maxVal, ymaxBox, maxVal));
				
				// aabb: x, y, z
				// min(xmax - xmin, zmax - zmin) = radius * 2
				// center top sphere = ymax - radius
				// center bottom sphere = ymin + radius
				// box (radius * radius * (ymax-ymin-2 * radius))

				PE::Handle hPhysicsComponent("PhysicsComponent", sizeof(PhysicsComponent));
				PhysicsComponent* pPhysicsComponent = new(hPhysicsComponent) PhysicsComponent(*m_pContext, m_arena, hPhysicsComponent, hMeshInstance, hSN);
				pPhysicsComponent->addDefaultComponents();
				//pPhysicsComponent->enableGravity();
				//pPhysicsComponent->m_collider = PhysicsComponent::ColliderType::SPHERE;
				//pPhysicsComponent->m_base = pMainSN->m_base;
				pMainSN->m_hPhysics = hPhysicsComponent;
				Matrix4x4 identity = Matrix4x4();
				identity.loadIdentity();

				// create sub comps
				PE::Handle hPhysicsComponentTop("PhysicsComponentTopSphere", sizeof(PhysicsComponent));
				PhysicsComponent* pPhysicsComponentTop = new(hPhysicsComponentTop) PhysicsComponent(*m_pContext, m_arena, hPhysicsComponentTop, hPhysicsComponent, Matrix4x4(centerTop), radius);

				PE::Handle hPhysicsComponentBot("PhysicsComponentBotSphere", sizeof(PhysicsComponent));
				PhysicsComponent* pPhysicsComponentBot = new(hPhysicsComponentBot) PhysicsComponent(*m_pContext, m_arena, hPhysicsComponentBot, hPhysicsComponent, Matrix4x4(centerBot), radius);

				PE::Handle hPhysicsComponentBox("PhysicsComponentBox", sizeof(PhysicsComponent));
				PhysicsComponent* pPhysicsComponentBox = new(hPhysicsComponentBox) PhysicsComponent(*m_pContext, m_arena, hPhysicsComponentBox, hPhysicsComponent, identity, boxAABB);

				pPhysicsComponent->m_subComps.add(hPhysicsComponentTop);
				pPhysicsComponent->m_subComps.add(hPhysicsComponentBot);
				pPhysicsComponent->m_subComps.add(hPhysicsComponentBox);
				// add skin to scene node
				pRotateSN->addComponent(hSkeletonInstance);

#if !APIABSTRACTION_D3D11
				{
					PE::Handle hMyGunMesh = PE::Handle("MeshInstance", sizeof(MeshInstance));
					MeshInstance* pGunMeshInstance = new(hMyGunMesh) MeshInstance(*m_pContext, m_arena, hMyGunMesh);

					pGunMeshInstance->addDefaultComponents();
					pGunMeshInstance->initFromFile(pEvt->m_gunMeshName, pEvt->m_gunMeshPackage, pEvt->m_threadOwnershipMask);

					// create a scene node for gun attached to a joint

					PE::Handle hMyGunSN = PE::Handle("SCENE_NODE", sizeof(JointSceneNode));
					JointSceneNode* pGunSN = new(hMyGunSN) JointSceneNode(*m_pContext, m_arena, hMyGunSN, 38);
					pGunSN->addDefaultComponents();

					// add gun to joint
					pGunSN->addComponent(hMyGunMesh);

					// add gun scene node to the skin
					pSkelInst->addComponent(hMyGunSN);
				}
#endif

				pMainSN->addComponent(hsSN);
			}

			m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(pEvt->m_threadOwnershipMask);

#if 1
			// add movement state machine to soldier npc
			PE::Handle hSoldierMovementSM("SoldierNPCMovementSM", sizeof(SoldierNPCMovementSM));
			SoldierNPCMovementSM* pSoldierMovementSM = new(hSoldierMovementSM) SoldierNPCMovementSM(*m_pContext, m_arena, hSoldierMovementSM);
			pSoldierMovementSM->addDefaultComponents();

			// add it to soldier NPC
			addComponent(hSoldierMovementSM);

			// add behavior state machine ot soldier npc
			PE::Handle hSoldierBheaviorSM("SoldierNPCBehaviorSM", sizeof(SoldierNPCBehaviorSM));
			SoldierNPCBehaviorSM* pSoldierBehaviorSM = new(hSoldierBheaviorSM) SoldierNPCBehaviorSM(*m_pContext, m_arena, hSoldierBheaviorSM, hSoldierMovementSM);
			pSoldierBehaviorSM->addDefaultComponents();

			// add it to soldier NPC
			addComponent(hSoldierBheaviorSM);

			StringOps::writeToString(pEvt->m_patrolWayPoint, pSoldierBehaviorSM->m_curPatrolWayPoint, 32);
			pSoldierBehaviorSM->m_havePatrolWayPoint = StringOps::length(pSoldierBehaviorSM->m_curPatrolWayPoint) > 0;

			// start the soldier
			pSoldierBehaviorSM->start();
#endif
		}

		void SoldierNPC::addDefaultComponents()
		{
			Component::addDefaultComponents();

			// custom methods of this component
		}

	}; // namespace Components
}; // namespace CharacterControl
