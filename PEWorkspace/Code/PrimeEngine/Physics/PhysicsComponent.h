#ifndef __pe_physicscomponent_h__
#define __pe_physicscomponent_h__

#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"

// Sibling/Children includes

namespace PE {
	namespace Components {

		struct PhysicsComponent : public Component
		{
			PE_DECLARE_CLASS(PhysicsComponent);

			enum ColliderType {
				BOX = 0,
				SPHERE = 1,
				CAPSULE = 2,
				ColliderType_Count
			};
			// Constructor -------------------------------------------------------------
			PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hMeshInstance, Handle hSceneNode);
			PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hRoot, Matrix4x4& offset, float radius);
			PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hRoot, Matrix4x4& offset, Array<Vector3>& aabb);

			void getPlanesForBox();

			virtual ~PhysicsComponent() {}

			virtual void addDefaultComponents();

			Handle m_hMesh; // handle to mesh
			Handle m_hSN; // handle to scene node
			Handle m_hMinst; // handle to mesh instance
			Handle m_hRoot; // root physics comp.
			bool m_root = true;
			bool m_gravity = false;
			bool m_static = false;
			bool m_rolledBack = false;
			Vector3 m_velocity;
			Vector3 m_aVelocity;
			Vector3 m_acceleration;
			Vector3 m_tForce;
			Array<Vector3> m_forces;
			Array<Vector3> m_normals;
			Array<Vector3> m_planeP;
			float m_angularAttenuation;
			float m_friction;
			float m_bounce;
			float m_radius;

			Matrix4x4 m_base; // make sure to always keep it world space
			Matrix4x4 m_world;

			Vector3 m_positionCache;
			float m_mass;
			ColliderType m_collider;
			Array<Vector3> m_aabb;
			Array<Handle> m_subComps;

			Vector3 m_noMove;
			Vector3 m_posCorrection;
			float m_timeLeft;

			void setPosition(const Vector3& nposition);
			void setVelocity(const Vector3& nvelocity);
			void setAVelocity(const Vector3& nvelocity);

			void addForce(const Vector3& forcev);
			void enableGravity();

			// update methods for next frame
			void updateForce(); // all force calculation excludes force from collision
			void updateVelocity(float dt);
			void updatePosition(float dt);
			void updateAVelocity(float dt);
			void updateRotation(float dt);
			void update(float dt);

			Vector3 getWorldPositionOffset();
			Vector3 getWorldPositionOffsetCache();

			// collision detection
			bool isColliding(PhysicsComponent* comp);
			bool isCollidingBoxSphere(PhysicsComponent* ball, PhysicsComponent* box);
			bool isCollidingSpheres(PhysicsComponent* ball1, PhysicsComponent* ball2);
			void correctMovements();
			//float getDiagonal();
			float getRadius();
			//void calculateOBB();

			// helper methods
			void goTo(Vector3 target, float speed, float range);
			bool isWithin(Vector3 target, float range);
			void turnTo(Vector3 target);
			void drawVolumes();
			void drawVolume();
			void drawSphere();
			void drawBox();

			void syncSceneNode();
			Vector3 getLocation();
			bool isMoving();
		};

	}; // namespace Components
}; // namespace PE
#endif


