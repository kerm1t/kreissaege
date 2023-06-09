// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLDISTVEC2LIN2_H
#define WMLDISTVEC2LIN2_H

#include "_WmlLine2.h"
#include "_WmlRay2.h"
#include "_WmlSegment2.h"

namespace Wml
{

// squared distance measurements

//template <class Real>
Vector2f LotFusspunkt (const Vector2f& rkPoint,
    const Line2f& rkLine);

template <class Real>
WML_ITEM Real SqrDistance (const Vector2<Real>& rkPoint,
    const Line2<Real>& rkLine, Real* pfParam = NULL);

template <class Real>
WML_ITEM Real SqrDistance (const Vector2<Real>& rkPoint,
    const Ray2<Real>& rkRay, Real* pfParam = NULL);

template <class Real>
WML_ITEM Real SqrDistance (const Vector2<Real>& rkPoint,
    const Segment2<Real>& rkSegment, Real* pfParam = NULL);


// distance measurements

template <class Real>
WML_ITEM Real Distance (const Vector2<Real>& rkPoint,
    const Line2<Real>& rkLine, Real* pfParam = NULL);

template <class Real>
WML_ITEM Real Distance (const Vector2<Real>& rkPoint,
    const Ray2<Real>& rkRay, Real* pfParam = NULL);

template <class Real>
WML_ITEM Real Distance (const Vector2<Real>& rkPoint,
    const Segment2<Real>& rkSegment, Real* pfParam = NULL);

}

#endif
