#ifndef __PYENGINE_2_0_PLANE_H__
#define __PYENGINE_2_0_PLANE_H__


// APIAbstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
// Windows
#if APIABSTRACTION_D3D9 | APIABSTRACTION_D3D11
#define _USE_MATH_DEFINES
#include <math.h>
#endif
// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"

// Sibling/Children includes
#include "MathHelpers.h"
#include "Vector3.h"

#if PE_PLAT_IS_WIN32
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

struct Plane {
  Vector3 m_normal;
  Vector3 m_bounds[2];
  bool m_isAABB;
  bool m_isBounded;
  PrimitiveTypes::Float32 m_d;

  Plane(float a, float b, float c, float d) {
    m_normal = Vector3(a, b, c);
    m_normal.normalize();
    m_d = d / m_normal.length();
    m_isAABB = false;
    m_isBounded = false;
  }

  Plane(Vector3 n, Vector3 p) {
    // given a point and normal
    m_normal = n;
    m_normal.normalize();

    m_d = m_normal.dotProduct(p);
    m_isAABB = false;
    m_isBounded = false;
  }

  Plane(Vector3 p1, Vector3 p2, Vector3 p3) {
    // given 3 points
    Vector3 v1 = p2 - p1;
    Vector3 v2 = p3 - p1;

    m_normal = v1.crossProduct(v2);
    m_normal.normalize();
    m_d = m_normal.dotProduct(p1);
    m_isAABB = false;
    m_isBounded = false;
  }

  Plane(Vector3 lb, Vector3 ub, Vector3 n) {
    // given bounds for aabb
    m_normal = n;
    m_normal.normalize();

    m_d = m_normal.dotProduct(lb);
    m_isBounded = true;
    m_isAABB = true;
    m_bounds[0] = lb;
    m_bounds[1] = ub;
  }

  bool isWithin(Vector3 p) {
    if (m_normal.dotProduct(p) + m_d == 0) {
      return p.m_x >= m_bounds[0].m_x && 
        p.m_x <= m_bounds[1].m_x &&
        p.m_y >= m_bounds[0].m_y && 
        p.m_y <= m_bounds[1].m_y &&
        p.m_z >= m_bounds[0].m_z && 
        p.m_z <= m_bounds[1].m_z;
    }
  }
}

#endif