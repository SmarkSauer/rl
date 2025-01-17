//
// Copyright (c) 2009, Markus Rickert
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#ifndef RL_MATH_TRANSFORMADDONS_H
#define RL_MATH_TRANSFORMADDONS_H

#ifdef DOXYGEN_SHOULD_PARSE_THIS
namespace Eigen { template<typename _Scalar, int _Dim, int _Mode, int _Options> class Transform {
#endif

template<typename OtherScalar, int OtherDim, int OtherMode, int OtherOptions>
inline
Scalar
distance(const Transform<OtherScalar, OtherDim, OtherMode, OtherOptions>& other, const Scalar& weight = 1) const
{
	using ::std::pow;
	using ::std::sqrt;
	
	Quaternion<Scalar> q1(rotation());
	Quaternion<Scalar> q2(other.rotation());
	
	return sqrt(
		pow(other(0, 3) - (*this)(0, 3), 2) +
		pow(other(1, 3) - (*this)(1, 3), 2) +
		pow(other(2, 3) - (*this)(2, 3), 2) +
		weight * pow(q1.angularDistance(q2), 2)
	);
}

/**
 * Calculate transformation from a linear and angular velocity vector.
 *
 * @param[in] useApproximation For rotations a, b, c smaller than a
 * few degrees, you can use a faster bi-linear approximation. For
 * rotations larger than 90 degrees, this approximation would be
 * completely wrong.
 */
template<typename OtherScalar1, int OtherDim, int OtherMode, int OtherOptions, typename OtherScalar2>
inline
void
fromDelta(const Transform<OtherScalar1, OtherDim, OtherMode, OtherOptions>& other, const Matrix<OtherScalar2, 6, 1>& delta, const bool& useApproximation = false)
{
	if (useApproximation)
	{
		(*this)(0, 0) = other(0, 0) - delta(5) * other(1, 0) + delta(4) * other(2, 0);
		(*this)(0, 1) = other(0, 1) - delta(5) * other(1, 1) + delta(4) * other(2, 1);
		(*this)(0, 2) = other(0, 2) - delta(5) * other(1, 2) + delta(4) * other(2, 2);
		(*this)(0, 3) = other(0, 3) + delta(0);
		(*this)(1, 0) = other(1, 0) + delta(5) * other(0, 0) - delta(3) * other(2, 0);
		(*this)(1, 1) = other(1, 1) + delta(5) * other(0, 1) - delta(3) * other(2, 1);
		(*this)(1, 2) = other(1, 2) + delta(5) * other(0, 2) - delta(3) * other(2, 2);
		(*this)(1, 3) = other(1, 3) + delta(1);
		(*this)(2, 0) = other(2, 0) - delta(4) * other(0, 0) + delta(3) * other(1, 0);
		(*this)(2, 1) = other(2, 1) - delta(4) * other(0, 1) + delta(3) * other(1, 1);
		(*this)(2, 2) = other(2, 2) - delta(4) * other(0, 2) + delta(3) * other(1, 2);
		(*this)(2, 3) = other(2, 3) + delta(2);
		(*this)(3, 0) = Scalar(0);
		(*this)(3, 1) = Scalar(0);
		(*this)(3, 2) = Scalar(0);
		(*this)(3, 3) = Scalar(1);
	}
	else
	{
		Transform<Scalar, Dim, Mode, Options> t;
		t.linear() = (AngleAxis<Scalar>(delta.segment(3, 3).norm(), delta.segment(3, 3).normalized())).toRotationMatrix();
		t.translation() = t.linear() * delta.segment(0, 3);
		(*this) = t * other;
	}
}

template<typename OtherScalar1, typename OtherScalar2, typename OtherScalar3, typename OtherScalar4>
inline
void
fromDenavitHartenbergPaul(const OtherScalar1& d, const OtherScalar2& theta, const OtherScalar3& a, const OtherScalar4& alpha)
{
	using ::std::cos;
	using ::std::sin;
	
	Scalar cosAlpha = cos(alpha);
	Scalar cosTheta = cos(theta);
	Scalar sinAlpha = sin(alpha);
	Scalar sinTheta = sin(theta);
	
	(*this)(0, 0) = cosTheta;
	(*this)(1, 0) = sinTheta;
	(*this)(2, 0) = Scalar(0);
	(*this)(3, 0) = Scalar(0);
	(*this)(0, 1) = -cosAlpha * sinTheta;
	(*this)(1, 1) = cosAlpha * cosTheta;
	(*this)(2, 1) = sinAlpha;
	(*this)(3, 1) = Scalar(0);
	(*this)(0, 2) = sinAlpha * sinTheta;
	(*this)(1, 2) = -sinAlpha * cosTheta;
	(*this)(2, 2) = cosAlpha;
	(*this)(3, 2) = Scalar(0);
	(*this)(0, 3) = a * cosTheta;
	(*this)(1, 3) = a * sinTheta;
	(*this)(2, 3) = d;
	(*this)(3, 3) = Scalar(1);
}

inline
Matrix<Scalar, 6, 1>
getDelta() const
{
	Matrix<Scalar, 6, 1> res;
	
	res(0) = (*this)(0, 3);
	res(1) = (*this)(1, 3);
	res(2) = (*this)(2, 3);
	res(3) = ((*this)(2, 1) - (*this)(1, 2)) * Scalar(0.5);
	res(4) = ((*this)(0, 2) - (*this)(2, 0)) * Scalar(0.5);
	res(5) = ((*this)(1, 0) - (*this)(0, 1)) * Scalar(0.5);
	
	return res;
}

template<typename OtherScalar>
inline
void
setDelta(const Matrix<OtherScalar, 6, 1>& delta)
{
	(*this)(0, 0) = Scalar(0);
	(*this)(0, 1) = -delta(5);
	(*this)(0, 2) = delta(4);
	(*this)(0, 3) = delta(0);
	(*this)(1, 0) = delta(5);
	(*this)(1, 1) = Scalar(0);
	(*this)(1, 2) = -delta(3);
	(*this)(1, 3) = delta(1);
	(*this)(2, 0) = -delta(4);
	(*this)(2, 1) = delta(3);
	(*this)(2, 2) = Scalar(0);
	(*this)(2, 3) = delta(2);
	(*this)(3, 0) = Scalar(0);
	(*this)(3, 1) = Scalar(0);
	(*this)(3, 2) = Scalar(0);
	(*this)(3, 3) = Scalar(1);
}

/**
 * Calculate the linear and angular velocity vector between two transformations.
 *
 * @param[in] useApproximation For rotations from t1 to t2 smaller
 * than a few degrees, you can use a faster bi-linear approximation.
 * (For a rotation angle < 1e-3, the approximated angular velocity
 * vector will be accurate up to 1e-9.) For rotations larger than 90
 * degrees, this approximation would be completely wrong.
 */
template<typename OtherScalar, int OtherDim, int OtherMode, int OtherOptions>
inline
Matrix<Scalar, 6, 1>
toDelta(const Transform<OtherScalar, OtherDim, OtherMode, OtherOptions>& other, const bool& useApproximation = false) const
{
	Matrix<Scalar, 6, 1> res;
	
	res(0) = other(0, 3) - (*this)(0, 3);
	res(1) = other(1, 3) - (*this)(1, 3);
	res(2) = other(2, 3) - (*this)(2, 3);
	
	if (useApproximation)
	{
		res(3) = ((*this)(1, 0) * other(2, 0) - (*this)(2, 0) * other(1, 0) + (*this)(1, 1) * other(2, 1) - (*this)(2, 1) * other(1, 1) + (*this)(1, 2) * other(2, 2) - (*this)(2, 2) * other(1, 2)) * Scalar(0.5);
		res(4) = ((*this)(2, 0) * other(0, 0) - (*this)(0, 0) * other(2, 0) + (*this)(2, 1) * other(0, 1) - (*this)(0, 1) * other(2, 1) + (*this)(2, 2) * other(0, 2) - (*this)(0, 2) * other(2, 2)) * Scalar(0.5);
		res(5) = ((*this)(0, 0) * other(1, 0) - (*this)(1, 0) * other(0, 0) + (*this)(0, 1) * other(1, 1) - (*this)(1, 1) * other(0, 1) + (*this)(0, 2) * other(1, 2) - (*this)(1, 2) * other(0, 2)) * Scalar(0.5);
	}
	else
	{
		AngleAxis<Scalar> r(Matrix<Scalar, 3, 3>(other.linear() * linear().transpose()));
		res.segment(3, 3) = r.angle() * r.axis();
	}
	
	return res;
}

template<typename OtherScalar1, typename OtherScalar2, typename OtherScalar3, typename OtherScalar4>
inline
void
toDenavitHartenbergPaul(OtherScalar1& d, OtherScalar2& theta, OtherScalar3& a, OtherScalar4& alpha) const
{
	using ::std::abs;
	using ::std::atan2;
	
	assert(abs((*this)(2, 0)) <= ::std::numeric_limits<Scalar>::epsilon());
	
	d = (*this)(2, 3);
	
	theta = atan2((*this)(1, 0), (*this)(0, 0));
	
	if (abs((*this)(0, 0)) <= ::std::numeric_limits<Scalar>::epsilon())
	{
		a = (*this)(1, 3) / (*this)(1, 0);
	}
	else if (abs((*this)(1, 0)) <= ::std::numeric_limits<Scalar>::epsilon())
	{
		a = (*this)(0, 3) / (*this)(0, 0);
	}
	else
	{
		a = ((*this)(1, 3) / (*this)(1, 0) + (*this)(0, 3) / (*this)(0, 0)) * Scalar(0.5);
	}
	
	alpha = atan2((*this)(2, 1), (*this)(2, 2));
}

#ifdef DOXYGEN_SHOULD_PARSE_THIS
} }
#endif

#ifdef RL_EIGEN_TRANSFORM_PLUGIN
#include RL_EIGEN_TRANSFORM_PLUGIN
#endif

#endif // RL_MATH_TRANSFORMADDONS_H
