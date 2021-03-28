// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "MeshManager.h"
// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

#include "PrimeEngine/Geometry/SkeletonCPU/SkeletonCPU.h"

#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Light.h"

// Sibling/Children includes
#include "Mesh.h"
#include "MeshInstance.h"
#include "Skeleton.h"
#include "SceneNode.h"
#include "DrawList.h"
#include "SH_DRAW.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

namespace PE {
namespace Components{

PE_IMPLEMENT_CLASS1(MeshManager, Component);
MeshManager::MeshManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
	: Component(context, arena, hMyself)
	, m_assets(context, arena, 256)
{
}

PE::Handle MeshManager::getAsset(const char *asset, const char *package, int &threadOwnershipMask)
{
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);
	
	int index = m_assets.findIndex(key);
	if (index != -1)
	{
		return m_assets.m_pairs[index].m_value;
	}
	Handle h;

	if (StringOps::endswith(asset, "skela"))
	{
		PE::Handle hSkeleton("Skeleton", sizeof(Skeleton));
		Skeleton *pSkeleton = new(hSkeleton) Skeleton(*m_pContext, m_arena, hSkeleton);
		pSkeleton->addDefaultComponents();

		pSkeleton->initFromFiles(asset, package, threadOwnershipMask);
		h = hSkeleton;
	}
	else if (StringOps::endswith(asset, "mesha"))
	{
		MeshCPU mcpu(*m_pContext, m_arena);
		mcpu.ReadMesh(asset, package, "");
		
		PE::Handle hMesh("Mesh", sizeof(Mesh));
		Mesh *pMesh = new(hMesh) Mesh(*m_pContext, m_arena, hMesh);
		pMesh->addDefaultComponents();

		pMesh->loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

#if PE_API_IS_D3D11
		// todo: work out how lods will work
		//scpu.buildLod();
#endif
        // generate collision volume here. or you could generate it in MeshCPU::ReadMesh()
        pMesh->m_performBoundingVolumeCulling = true; // will now perform tests for this mesh
		computeAABB(hMesh);
		h = hMesh;
	}


	PEASSERT(h.isValid(), "Something must need to be loaded here");

	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
	return h;
}

void MeshManager::registerAsset(const PE::Handle &h)
{
	static int uniqueId = 0;
	++uniqueId;
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "__generated_%d", uniqueId);
	
	int index = m_assets.findIndex(key);
	PEASSERT(index == -1, "Generated meshes have to be unique");
	
	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
}

void MeshManager::computeAABB(Handle hMesh)
{
	Mesh* pMesh = hMesh.getObject<Mesh>();
	if (pMesh->m_aabb.m_size == 0)
	{
		PositionBufferCPU* pPoss = pMesh->m_hPositionBufferCPU.getObject<PositionBufferCPU>();
		int totalSize = pPoss->m_values.m_size / 3;
		float xMax = FLT_MIN;
		float yMax = FLT_MIN;
		float zMax = FLT_MIN;
		float xMin = FLT_MAX;
		float yMin = FLT_MAX;
		float zMin = FLT_MAX;

		for (int i = 0; i < totalSize; ++i)
		{
			xMax = max(xMax, pPoss->m_values[i * 3]);
			yMax = max(yMax, pPoss->m_values[i * 3 + 1]);
			zMax = max(zMax, pPoss->m_values[i * 3 + 2]);

			xMin = min(xMin, pPoss->m_values[i * 3]);
			yMin = min(yMin, pPoss->m_values[i * 3 + 1]);
			zMin = min(zMin, pPoss->m_values[i * 3 + 2]);
		}
		pMesh->m_aabb.add(Vector3(xMin, yMin, zMin));
		pMesh->m_aabb.add(Vector3(xMax, yMax, zMax));

		//// calculate normals
		//Vector3 dx = Vector3(1, 0, 0);
		//Vector3 dy = Vector3(0, 1, 0);
		//Vector3 dz = Vector3(0, 0, 1);

		//pMesh->m_normals.add(-dz); // front
		//pMesh->m_normals.add(dx); // right
		//pMesh->m_normals.add(dy); // top
		//pMesh->m_normals.add(-dx); // left
		//pMesh->m_normals.add(-dy); // bottom
		//pMesh->m_normals.add(dz); // far

		//pMesh->m_planeP.add(Vector3(xMin, yMin, zMin));
		//pMesh->m_planeP.add(Vector3(xMax, yMax, zMax));
		//pMesh->m_planeP.add(Vector3(xMax, yMax, zMax));
		//pMesh->m_planeP.add(Vector3(xMin, yMin, zMin));
		//pMesh->m_planeP.add(Vector3(xMin, yMin, zMin));
		//pMesh->m_planeP.add(Vector3(xMax, yMax, zMax));
	}
	// skip if already computed
}

}; // namespace Components
}; // namespace PE
