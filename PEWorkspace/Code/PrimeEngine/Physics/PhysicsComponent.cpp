#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes

#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PhysicsComponent.h"

// Sibling/Children includes
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/Mesh.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PhysicsManager.h"

#undef PI
#define PI (3.14159265358979323846)

namespace PE {
	namespace Components {

		PE_IMPLEMENT_CLASS1(PhysicsComponent, Component);

		PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hMeshInstance, Handle hSceneNode)
			: Component(context, arena, hMyself)
			, m_hMinst(hMeshInstance)
			, m_forces(context, arena, 8)
			, m_aabb(context, arena, 2)
			, m_subComps(context, arena, 8)
			, m_planeP(context, arena, 6)
			, m_normals(context, arena, 6)
		{
			// get mesh handle
			MeshInstance* pmins = m_hMinst.getObject<MeshInstance>();
			m_hMesh = pmins->m_hAsset;
			Mesh* pMesh = m_hMesh.getObject<Mesh>();
			m_aabb.add(Vector3(pMesh->m_aabb[0])); // min
			m_aabb.add(Vector3(pMesh->m_aabb[1])); // max
			// get scene node handle
			m_hSN = hSceneNode;
			SceneNode* pSN = m_hSN.getObject<SceneNode>();

			// add to list of physics components
			PhysicsManager::Instance()->addComponent(m_hMyself);
			m_root = true;
			m_hRoot = m_hMyself;
			// other states
			m_mass = 10; // kg
			m_velocity = Vector3();
			m_base = pSN->m_worldTransform * pSN->m_base; // has to be in world
			m_world = Matrix4x4();
			m_positionCache = m_base.getPos();
			m_tForce = Vector3();
			m_aVelocity = Vector3();
			m_acceleration = Vector3();
			m_posCorrection = Vector3();
			m_collider = BOX;
			m_gravity = false;
			m_static = true;
			m_angularAttenuation = 0;
			m_friction = 0;
			m_bounce = 0;
			m_timeLeft = 1;
			getPlanesForBox();
		}

		PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hRoot, Matrix4x4& offset, float radius)
			: Component(context, arena, hMyself)
			, m_forces(context, arena, 8)
			, m_aabb(context, arena, 2)
			, m_subComps(context, arena, 8)
			, m_planeP(context, arena, 6)
			, m_normals(context, arena, 6)
		{
			// do not add to list of physics components
			//PhysicsManager::Instance()->addComponent(m_hMyself); // dont have to
			m_root = false;
			m_hRoot = hRoot;
			PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
			// other states
			m_mass = 10; // kg
			m_velocity = Vector3();
			//m_base = base;
			m_base = offset; // need to add on top of parent object
			m_world = pRoot->m_base;
			m_positionCache = m_base.getPos();
			m_tForce = Vector3();
			m_aVelocity = Vector3();
			m_acceleration = Vector3();
			m_posCorrection = Vector3();
			m_collider = SPHERE;
			m_gravity = false;
			m_static = true;
			m_angularAttenuation = 0;
			m_friction = 0;
			m_bounce = 0;
			m_timeLeft = 1;
			m_radius = radius;
		}

		PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hRoot, Matrix4x4& offset, Array<Vector3>& aabb)
			: Component(context, arena, hMyself)
			, m_forces(context, arena, 8)
			, m_aabb(context, arena, 2)
			, m_subComps(context, arena, 8)
			, m_planeP(context, arena, 6)
			, m_normals(context, arena, 6)
		{
			// do not add to list of physics components
			//PhysicsManager::Instance()->addComponent(m_hMyself); // dont have to
			m_root = false;
			m_hRoot = hRoot;
			PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
			// other states
			m_mass = 10; // kg
			m_velocity = Vector3();
			//m_base = base;
			m_base = offset; // need to add on top of parent object
			m_world = pRoot->m_base;
			m_positionCache = m_base.getPos();
			m_tForce = Vector3();
			m_aVelocity = Vector3();
			m_acceleration = Vector3();
			m_posCorrection = Vector3();
			m_collider = BOX;
			m_gravity = false;
			m_static = true;
			m_angularAttenuation = 0;
			m_friction = 0;
			m_bounce = 0;
			m_timeLeft = 1;
			m_aabb.add(aabb[0]);
			m_aabb.add(aabb[1]);
			getPlanesForBox();
		}

		void PhysicsComponent::getPlanesForBox()
		{
			float xMin = m_aabb[0].m_x;
			float yMin = m_aabb[0].m_y;
			float zMin = m_aabb[0].m_z;
			float xMax = m_aabb[1].m_x;
			float yMax = m_aabb[1].m_y;
			float zMax = m_aabb[1].m_z;
			// calculate normals
			Vector3 dx = Vector3(1, 0, 0);
			Vector3 dy = Vector3(0, 1, 0);
			Vector3 dz = Vector3(0, 0, 1);

			m_normals.add(-dz); // front
			m_normals.add(dx); // right
			m_normals.add(dy); // top
			m_normals.add(-dx); // left
			m_normals.add(-dy); // bottom
			m_normals.add(dz); // far

			m_planeP.add(Vector3(xMin, yMin, zMin));
			m_planeP.add(Vector3(xMax, yMax, zMax));
			m_planeP.add(Vector3(xMax, yMax, zMax));
			m_planeP.add(Vector3(xMin, yMin, zMin));
			m_planeP.add(Vector3(xMin, yMin, zMin));
			m_planeP.add(Vector3(xMax, yMax, zMax));

			// calculate radius
			m_radius = (yMax - yMin) / 2;
		}

		void PhysicsComponent::addForce(const Vector3& forcev)
		{
			m_forces.add(forcev);
		}

		void PhysicsComponent::enableGravity()
		{
			m_gravity = true;
			m_static = false;
		}

		void PhysicsComponent::addDefaultComponents()
		{
			Component::addDefaultComponents();
		}

		void PhysicsComponent::setPosition(const Vector3& nposition)
		{
			m_positionCache = m_base.getPos();
			m_base.setPos(nposition);

		}

		void PhysicsComponent::setVelocity(const Vector3& nvelocity)
		{
			m_velocity = nvelocity;
		}

		void PhysicsComponent::setAVelocity(const Vector3& nvelocity)
		{
			m_aVelocity = nvelocity;
		}

		void PhysicsComponent::updateForce()
		{
			if (!m_root) return;
			// calculate gravity
			m_tForce = Vector3(0, 0, 0);
			if (m_gravity) {
				m_tForce += Vector3(0, -GRAVITY * m_mass, 0);
			}
			// accumulate forces
			for (int iforce = 0; iforce < m_forces.m_size; ++iforce) {
				m_tForce += m_forces[iforce];
			}
			// calculate magnus force?
			if (m_aVelocity.lengthSqr() > 0) {
				float scale = pow(PI, 2) * pow(getRadius() / 10, 3) * 1.225;
				Vector3 mForce = m_velocity.crossProduct(m_aVelocity) * scale * m_velocity.length();
				m_tForce += mForce;
			}
		}

		void PhysicsComponent::updateVelocity(float dt)
		{
			if (m_tForce.lengthSqr() > 0)
				m_velocity += ((dt / m_mass) * m_tForce);
		}

		void PhysicsComponent::updatePosition(float dt)
		{
			if (m_velocity.lengthSqr() > 0)
				setPosition(m_base.getPos() + dt * m_velocity);
		}

		void PhysicsComponent::updateAVelocity(float dt)
		{
			if (m_aVelocity.lengthSqr() > 0)
				m_aVelocity *= (1 - m_angularAttenuation * dt);
		}

		void PhysicsComponent::updateRotation(float dt)
		{
			if (m_aVelocity.lengthSqr() > 0) {
				Vector3 av = m_aVelocity;
				av.normalize();
				m_base.turnAboutAxis(m_aVelocity.length() * dt * 2 * PI, av);
			}
		}

		void PhysicsComponent::update(float dt)
		{
			// calculate for next frame
			if (m_static) return;
			updateForce(); // all force calculation excludes force from collision
			updateVelocity(dt);
			updateAVelocity(dt);
			updateRotation(dt);
			updatePosition(dt);
		}

		Vector3 PhysicsComponent::getWorldPositionOffset()
		{
			if (m_root) return Vector3(0, 0, 0);
			PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
			return pRoot->m_base.getPos();
		}

		Vector3 PhysicsComponent::getWorldPositionOffsetCache()
		{
			if (m_root) return Vector3(0, 0, 0);
			PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
			return pRoot->m_positionCache;
		}

		bool PhysicsComponent::isColliding(PhysicsComponent* other)
		{
			if (m_subComps.m_size == 0 && other->m_subComps.m_size == 0) {
				if ((m_collider == BOX && other->m_collider == SPHERE) || (m_collider == SPHERE && other->m_collider == BOX))
				{
					PhysicsComponent* box = m_collider == BOX ? this : other;
					PhysicsComponent* ball = m_collider == BOX ? other : this;
					return isCollidingBoxSphere(ball, box);
				}
				else if (m_collider == SPHERE && other->m_collider == SPHERE) {
					// assuming the same mass
					PhysicsComponent* ball1 = this;
					PhysicsComponent* ball2 = other;
					return isCollidingSpheres(ball1, ball2);
				}
				return false;
			}

			bool ret = false;
			if (m_subComps.m_size > 0) {
				for (int i = 0; i < m_subComps.m_size; ++i) {
					PhysicsComponent* comp = m_subComps[i].getObject<PhysicsComponent>();
					if (other->m_subComps.m_size > 0) {
						for (int j = 0; j < other->m_subComps.m_size; ++j) {
							PhysicsComponent* comp2 = other->m_subComps[j].getObject<PhysicsComponent>();
							ret = comp->isColliding(comp2) || ret;
						}
					}
					else {
						ret = comp->isColliding(other) || ret;
					}

				}
			}
			else {
				if (other->m_subComps.m_size > 0) {
					for (int j = 0; j < other->m_subComps.m_size; ++j) {
						PhysicsComponent* comp2 = other->m_subComps[j].getObject<PhysicsComponent>();
						ret = isColliding(comp2) || ret;
					}
				}
				else {
					ret = isColliding(other) || ret;
				}
			}

			return ret;
		}

		bool PhysicsComponent::isCollidingBoxSphere(PhysicsComponent* ball, PhysicsComponent* box) {
			// only moving ball and static box
			bool colliding = false;
			
			Mesh* ballMesh = ball->m_hMesh.getObject<Mesh>();
			float radius = ball->m_radius;
			Vector3 center = ball->m_base.getPos() + ball->getWorldPositionOffset();
			Vector3 centerPrev = ball->m_positionCache;
			if (!ball->m_root) {
				centerPrev = ball->m_base.getPos() + ball->getWorldPositionOffsetCache();
			}
			//Matrix4x4 toBox = box->m_base.inverse();
			Matrix4x4 boxBase = box->m_base;
			boxBase.setPos(boxBase.getPos() + box->getWorldPositionOffset());
			Matrix4x4 toBox = boxBase.inverse();

			Vector3 mins = box->m_aabb[0];
			Vector3 maxs = box->m_aabb[1];
			Vector3 p1 = toBox * center;
			Vector3 p0 = toBox * centerPrev;

			// check penetration for each side
			Vector3 target = p1; // in box coord
			Vector3 v = p1 - p0;
			Vector3 noMove = Vector3();
			float tMin = 2.0f;
			for (int iSide = 0; iSide < 6; ++iSide) {
				Vector3 normal = box->m_normals[iSide];
				Vector3 p = box->m_planeP[iSide];
				Vector3 pTangentDiff = normal * radius;
				Vector3 pTangent = p + pTangentDiff;
				float a0 = normal.dotProduct(p0 - p);
				float a1 = normal.dotProduct(p1 - p);

				if (a0 * a1 > 0 && abs(a0) > radius && abs(a1) > radius) {
					continue;
				}

				// plane: (q - p).n = 0
				// line: q = p0 + t(p1 - p0)

				// (p0 + t(p1 - p0) - p).n = 0 => (p0 - p).n + t(p1-p0).n = 0
				// (p - p0).n / (p1 - p0).n = t
				float t = (pTangent - p0).dotProduct(normal) /
					v.dotProduct(normal);

				// line equation: p = p0 + t(p1 - p0)
				t = min(1.0f, max(0.0f, t));
				Vector3 intersectionP = p0 + t * v;
				// check P is within
				if (intersectionP.m_x >= (mins.m_x - radius) &&
					intersectionP.m_y >= (mins.m_y - radius) &&
					intersectionP.m_z >= (mins.m_z - radius) &&
					intersectionP.m_x <= (maxs.m_x + radius) &&
					intersectionP.m_y <= (maxs.m_y + radius) &&
					intersectionP.m_z <= (maxs.m_z + radius) &&
					/*(t < tMin || (tMin == 1.0f && t == 1.0f))*/
					t < tMin
					) {
					tMin = t;
					colliding = true;
					// location correction
					Vector3 disNormal = (intersectionP - p0).projectionOnVector(normal);
					target = p0 + disNormal;
					noMove = normal;
				}
			}

			if (colliding)
			{
				// only correct movements for root comps
				if (!ball->m_root) return true;
				// re calculate position & velocity vectors for sphere
				Vector3 u1 = box->m_base.getU();
				Vector3 v1 = box->m_base.getV();
				Vector3 n1 = box->m_base.getN();
				u1.normalize();
				v1.normalize();
				n1.normalize();
				Matrix3x3 transform = Matrix3x3(u1, v1, n1);
				ball->m_posCorrection = boxBase * target;
				noMove = transform * noMove;
				if (ball->m_bounce <= 0) {
					ball->setVelocity(ball->m_velocity - ball->m_velocity.projectionOnVector(noMove));
					ball->m_timeLeft = 1;
				}
				else {
					// only calculate sphere bouncing => not considering mass
					//Vector3 tMomentum = ball->m_mass * ball->m_velocity;
					
					//if (ball->m_velocity.length() < 1) vMomentum = 0;
					Vector3 uMomentum = ball->m_velocity;
					float vMomentum = uMomentum.length() * ball->m_bounce;
					uMomentum.normalize();
					Vector3 projectionVector = uMomentum.projectionOnVector(noMove); // vector othorgonal to bouncing surface
					Vector3 rMomentum = projectionVector * -2 + uMomentum;

					// add angular velocity attenuation
					Vector3 spinVelocity = Vector3(0, 0, 0);
					if (ball->m_aVelocity.lengthSqr() > 0.5) {
						/*ball->setAVelocity(ball->m_aVelocity * (1 - ball->m_bounce * 0.5));*/
						ball->setAVelocity(ball->m_aVelocity * (ball->m_bounce));
						spinVelocity = ball->m_velocity.crossProduct(ball->m_aVelocity) * 0.1;
					}

					//add rolling
					spinVelocity += -ball->m_aVelocity.crossProduct(noMove) * ball->m_radius;

					ball->setVelocity(rMomentum * vMomentum + spinVelocity);
				}
				

				ball->m_timeLeft = 1;
				return true;
			}
			return false;
		}

		bool PhysicsComponent::isCollidingSpheres(PhysicsComponent* ball1, PhysicsComponent* ball2) {
			Mesh* ball1Mesh = ball1->m_hMesh.getObject<Mesh>();
			float radius1 = ball1->m_radius;
			Vector3 center1 = ball1->m_base.getPos() + ball1->getWorldPositionOffset();
			Vector3 centerPrev1 = ball1->m_positionCache;
			if (!ball1->m_root) {
				centerPrev1 = ball1->m_base.getPos() + ball1->getWorldPositionOffsetCache();
			}

			Mesh* ball2Mesh = ball2->m_hMesh.getObject<Mesh>();
			float radius2 = ball2->m_radius;
			Vector3 center2 = ball2->m_base.getPos();
			Vector3 centerPrev2 = ball2->m_positionCache;
			if (!ball2->m_root) {
				centerPrev2 = ball2->m_base.getPos() + ball2->getWorldPositionOffsetCache();
			}

			// l1 = p0 + (p1 - p0) * m
			// l2 = q0 + (q1 - q0) * m
			Vector3 v1 = centerPrev2 - centerPrev1;
			Vector3 v2 = center2 - center1;
			float a = (v2 - v1).lengthSqr();
			float b = 2 * v1.dotProduct(v2 - v1);
			float c = v1.lengthSqr() - (radius1 + radius2) * (radius1 + radius2);
			float detSqr = b * b - 4 * a * c;

			if (a == 0 || detSqr < 0) {
				return false;
			}
			// there is root / collision
			// plus or minus need to determin which one is the real one
			float root1 = (-b - sqrt(detSqr)) / (2 * a);
			float root2 = (-b + sqrt(detSqr)) / (2 * a);
			float m = 0;
			if (root1 < 0) {
				m = 0;
			}
			else if (root1 > 1) {
				m = 1;
			}
			else {
				m = root1;
			}
			// assert m in [0, 1]
			Vector3 l1 = centerPrev1 + m * (center1 - centerPrev1);
			Vector3 l2 = centerPrev2 + m * (center2 - centerPrev2);
			// asset length to be radius
			Vector3 collisionVector = l2 - l1;
			float distance = collisionVector.length();
			if (distance > (radius1 + radius2)) return false;
			collisionVector.normalize();

			if (ball1->m_root && ball2->m_root) {
				// calculate velocity after collision
				Vector3 v1x = ball1->m_velocity.projectionOnVector(collisionVector);
				Vector3 v1y = ball1->m_velocity - v1x;
				Vector3 v2x = ball2->m_velocity.projectionOnVector(collisionVector);
				Vector3 v2y = ball2->m_velocity - v2x;
				// conservation of momentum in collision axis
				// assume 100% energy are conserved???
				Vector3 totalMomentum = v1x + v2x;
				float c1 = totalMomentum.dotProduct(collisionVector);
				float c2 = (v1x.lengthSqr() + v2x.lengthSqr()) * 1; // can add factor to represent energy loss

				a = 2;
				b = -2 * c1;
				c = c1 * c1 - c2;
				detSqr = b * b - 4 * a * c;
				if (a == 0 || detSqr < 0) {
					PEINFO("determinant less than 0 when determining collision velocity");
					return false;
				}
				// c2 <= (v1x^2 + v2x^2) = v1fx^2 + v2fx^2
				// c1 <= (v1x + v2x) = v1fx + v2fx
				root1 = (-b - sqrt(detSqr)) / (2 * a);
				root2 = (-b + sqrt(detSqr)) / (2 * a);
				Vector3 v2fx = root2 * collisionVector;
				Vector3 v1fx = (c1 - root2) * collisionVector; // vector direction and velocity could be incorrect
				// get the final velocity
				Vector3 v1f = v1y + v1fx;
				Vector3 v2f = v2y + v2fx;

				// take account for sub comps
				ball1->setVelocity(v1f);
				ball1->m_posCorrection = l1;
				ball1->m_timeLeft = 1 - m;
				ball2->setVelocity(v2f);
				ball2->m_posCorrection = l2;
				ball2->m_timeLeft = 1 - m;
			}
			else {
				//PhysicsComponent* nonRoot = ball1->m_root ? ball2 : ball1;
				PhysicsComponent* rootBall = ball1->m_root ? ball1 : ball2;
				
				// only calculate for root ball
				Vector3 vx = rootBall->m_velocity.projectionOnVector(collisionVector);
				Vector3 vy = rootBall->m_velocity - vx;

				rootBall->setVelocity(-vx + vy);
				rootBall->m_posCorrection = ball1->m_root ? l1 : l2;
				rootBall->m_timeLeft = 1 - m; 
			}
			return true;
		}

		void PhysicsComponent::correctMovements()
		{
			m_rolledBack = true;
			m_base.setPos(m_posCorrection);
			// Do we need to recalculate force and velocity?
		}

		float PhysicsComponent::getRadius()
		{
			return m_radius;
		}

		void PhysicsComponent::goTo(Vector3 target, float speed, float range)
		{
			Vector3 dir = (target - m_base.getPos());
			dir.m_y = 0; // make sure no vertical movement;
			m_base.turnTo(target);
			float dist = dir.lengthSqr();
			
			if (isWithin(target, range))
			{
				// already there
				setVelocity(Vector3(0, 0, 0));
				return;
			}

			dir.normalize();
			dir *= speed;
			dir.m_y = m_velocity.m_y;
			setVelocity(dir);
		}

		bool PhysicsComponent::isWithin(Vector3 target, float range)
		{
			return (target - m_base.getPos()).lengthSqr() <= range;
		}

		void PhysicsComponent::turnTo(Vector3 target)
		{
			m_base.turnTo(target);
		}

		void PhysicsComponent::drawVolumes()
		{
			if (m_subComps.m_size > 0) {
				for (int i = 0; i < m_subComps.m_size; ++i) {
					PhysicsComponent* comp = m_subComps[i].getObject<PhysicsComponent>();
					comp->drawVolume();
				}
			}
			else {
				drawVolume();
			}
		}

		void PhysicsComponent::drawVolume()
		{
			if (m_collider == SPHERE) {
				drawSphere();
			}
			else {
				drawBox();
			}
		}

		void PhysicsComponent::drawSphere()
		{
			Matrix4x4 transform = Matrix4x4();
			transform.loadIdentity();
			if (!m_root) {
				PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
				transform = pRoot->m_base;
			}
			Vector3 center = transform * m_base.getPos();
			float drawArray[36] = {
				center.m_x - m_radius, center.m_y, center.m_z, 1.0, 0.0, 0.0, center.m_x + m_radius, center.m_y, center.m_z, 1.0, 0.0, 0.0, // x - red
				center.m_x, center.m_y - m_radius, center.m_z, 0.0, 1.0, 0.0, center.m_x, center.m_y + m_radius, center.m_z, 0.0, 1.0, 0.0, // y - green
				center.m_x, center.m_y, center.m_z - m_radius, 0.0, 0.0, 1.0, center.m_x, center.m_y, center.m_z + m_radius, 0.0, 0.0, 1.0  // z - red
			};

			DebugRenderer::Instance()->createLineMesh(false, Matrix4x4(), drawArray, 6, 0, 1.0f);
		}

		void PhysicsComponent::drawBox()
		{
			Matrix4x4 transform = Matrix4x4();
			transform.loadIdentity();
			if (!m_root) {
				PhysicsComponent* pRoot = m_hRoot.getObject<PhysicsComponent>();
				transform = pRoot->m_base;
			}
			transform = transform * m_base;
			Vector3 mins = m_aabb[0];
			Vector3 maxs = m_aabb[1];
			Vector3 worldMin = transform * m_aabb[0];
			Vector3 worldMax = transform * m_aabb[1];

			Vector3 worldP1 = transform * Vector3(mins.m_x, mins.m_y, mins.m_z);
			Vector3 worldP2 = transform * Vector3(maxs.m_x, mins.m_y, mins.m_z);
			Vector3 worldP3 = transform * Vector3(mins.m_x, maxs.m_y, mins.m_z);
			Vector3 worldP4 = transform * Vector3(maxs.m_x, maxs.m_y, mins.m_z);

			Vector3 worldP5 = transform * Vector3(mins.m_x, mins.m_y, maxs.m_z);
			Vector3 worldP6 = transform * Vector3(maxs.m_x, mins.m_y, maxs.m_z);
			Vector3 worldP7 = transform * Vector3(mins.m_x, maxs.m_y, maxs.m_z);
			Vector3 worldP8 = transform * Vector3(maxs.m_x, maxs.m_y, maxs.m_z);

			float drawArray[] = {
				worldP1.m_x, worldP1.m_y, worldP1.m_z, 1, 1, 1, worldP2.m_x, worldP2.m_y, worldP2.m_z, 1, 1, 1,
				worldP3.m_x, worldP3.m_y, worldP3.m_z, 1, 1, 1, worldP4.m_x, worldP4.m_y, worldP4.m_z, 1, 1, 1,
				worldP5.m_x, worldP5.m_y, worldP5.m_z, 1, 1, 1, worldP6.m_x, worldP6.m_y, worldP6.m_z, 1, 1, 1,
				worldP7.m_x, worldP7.m_y, worldP7.m_z, 1, 1, 1, worldP8.m_x, worldP8.m_y, worldP8.m_z, 1, 1, 1,

				worldP1.m_x, worldP1.m_y, worldP1.m_z, 1, 1, 1, worldP3.m_x, worldP3.m_y, worldP3.m_z, 1, 1, 1,
				worldP2.m_x, worldP2.m_y, worldP2.m_z, 1, 1, 1, worldP4.m_x, worldP4.m_y, worldP4.m_z, 1, 1, 1,
				worldP5.m_x, worldP5.m_y, worldP5.m_z, 1, 1, 1, worldP7.m_x, worldP7.m_y, worldP7.m_z, 1, 1, 1,
				worldP6.m_x, worldP6.m_y, worldP6.m_z, 1, 1, 1, worldP8.m_x, worldP8.m_y, worldP8.m_z, 1, 1, 1,

				worldP1.m_x, worldP1.m_y, worldP1.m_z, 1, 1, 1, worldP5.m_x, worldP5.m_y, worldP5.m_z, 1, 1, 1,
				worldP2.m_x, worldP2.m_y, worldP2.m_z, 1, 1, 1, worldP6.m_x, worldP6.m_y, worldP6.m_z, 1, 1, 1,
				worldP3.m_x, worldP3.m_y, worldP3.m_z, 1, 1, 1, worldP7.m_x, worldP7.m_y, worldP7.m_z, 1, 1, 1,
				worldP4.m_x, worldP4.m_y, worldP4.m_z, 1, 1, 1, worldP8.m_x, worldP8.m_y, worldP8.m_z, 1, 1, 1,
			};

			DebugRenderer::Instance()->createLineMesh(false, Matrix4x4(), drawArray, 24, 0, 1.0f);
		}

		void PhysicsComponent::syncSceneNode()
		{
			if (!m_root) return;
			SceneNode* node = m_hSN.getObject<SceneNode>();
			node->m_base = m_base;
		}

		Vector3 PhysicsComponent::getLocation()
		{
			return m_base.getPos();
		}

		bool PhysicsComponent::isMoving()
		{
			return !m_static && m_velocity.lengthSqr() > 0;
		}

	}; // namespace Components
}; // namespace PE
