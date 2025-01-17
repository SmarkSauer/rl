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

#include "Body.h"
#include "Exception.h"
#include "Joint.h"
#include "Model.h"
#include "World.h"

namespace rl
{
	namespace mdl
	{
		Model::Model() :
			bodies(),
			elements(),
			frames(),
			gammaPosition(),
			gammaVelocity(),
			home(),
			invGammaPosition(),
			invGammaVelocity(),
			joints(),
			leaves(),
			manufacturer(),
			name(),
			root(0),
			tools(),
			transforms(),
			tree(),
			randDistribution(0, 1),
			randEngine(::std::random_device()())
		{
		}
		
		Model::~Model()
		{
		}
		
		void
		Model::add(Frame* frame)
		{
			this->add(::std::shared_ptr<Frame>(frame));
		}
		
		void
		Model::add(const ::std::shared_ptr<Frame>& frame)
		{
			Vertex vertex = ::boost::add_vertex(this->tree);
			frame->setVertexDescriptor(vertex);
			this->tree[vertex] = frame;
			
			if (::std::dynamic_pointer_cast<World>(frame))
			{
				this->root = vertex;
			}
		}
		
		void
		Model::add(Transform* transform, const Frame* a, const Frame* b)
		{
			this->add(::std::shared_ptr<Transform>(transform), a, b);
		}
		
		void
		Model::add(const ::std::shared_ptr<Transform>& transform, const Frame* a, const Frame* b)
		{
			Edge edge = ::boost::add_edge(a->getVertexDescriptor(), b->getVertexDescriptor(), this->tree).first;
			transform->setEdgeDescriptor(edge);
			this->tree[edge] = transform;
		}
		
		bool
		Model::areColliding(const ::std::size_t& i, const ::std::size_t& j) const
		{
			assert(i < this->bodies.size());
			assert(j < this->bodies.size());
			
			return this->bodies[i]->getCollision(this->bodies[j]) || this->bodies[j]->getCollision(this->bodies[i]);
		}
		
		::rl::math::Vector
		Model::generatePositionGaussian(const ::rl::math::Vector& mean, const ::rl::math::Vector& sigma)
		{
			::rl::math::Vector rand(this->getDof());
			
			for (::std::size_t i = 0; i < this->getDof(); ++i)
			{
				rand(i) = this->rand();
			}
			
			return this->generatePositionGaussian(rand, mean, sigma);
		}
		
		::rl::math::Vector
		Model::generatePositionGaussian(const ::rl::math::Vector& rand, const ::rl::math::Vector& mean, const ::rl::math::Vector& sigma) const
		{
			::rl::math::Vector q(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0, k = 0; i < this->joints.size(); k += this->joints[i]->getDof(), j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->generatePositionGaussian(
					rand.segment(k, this->joints[i]->getDof()),
					mean.segment(j, this->joints[i]->getDofPosition()),
					sigma.segment(k, this->joints[i]->getDof()),
					q.segment(j, this->joints[i]->getDofPosition())
				);
			}
			
			return q;
		}
		
		::rl::math::Vector
		Model::generatePositionUniform()
		{
			::rl::math::Vector rand(this->getDof());
			
			for (::std::size_t i = 0; i < this->getDof(); ++i)
			{
				rand(i) = this->rand();
			}
			
			return this->generatePositionUniform(rand);
		}
		
		::rl::math::Vector
		Model::generatePositionUniform(const ::rl::math::Vector& rand) const
		{
			::rl::math::Vector q(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0, k = 0; i < this->joints.size(); k += this->joints[i]->getDof(), j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->generatePositionUniform(
					rand.segment(k, this->joints[i]->getDof()),
					q.segment(j, this->joints[i]->getDofPosition())
				);
			}
			
			return q;
		}
		
		::rl::math::Vector
		Model::generatePositionUniform(const ::rl::math::Vector& min, const ::rl::math::Vector& max)
		{
			::rl::math::Vector rand(this->getDof());
			
			for (::std::size_t i = 0; i < this->getDof(); ++i)
			{
				rand(i) = this->rand();
			}
			
			return this->generatePositionUniform(rand, min, max);
		}
		
		::rl::math::Vector
		Model::generatePositionUniform(const ::rl::math::Vector& rand, const ::rl::math::Vector& min, const ::rl::math::Vector& max) const
		{
			::rl::math::Vector q(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0, k = 0; i < this->joints.size(); k += this->joints[i]->getDof(), j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->generatePositionUniform(
					rand.segment(k, this->joints[i]->getDof()),
					min.segment(k, this->joints[i]->getDof()),
					max.segment(k, this->joints[i]->getDof()),
					q.segment(j, this->joints[i]->getDofPosition())
				);
			}
			
			return q;
		}
		
		::rl::math::Vector
		Model::getAcceleration() const
		{
			::rl::math::Vector qdd(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				qdd.segment(j, this->joints[i]->getDof()) = this->joints[i]->getAcceleration();
			}
			
			return this->invGammaVelocity * qdd;
		}
		
		::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1>
		Model::getAccelerationUnits() const
		{
			::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1> units(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				units.segment(j, this->joints[i]->getDof()) = this->joints[i]->getAccelerationUnits();
			}
			
			return units;
		}
		
		::std::size_t
		Model::getBodies() const
		{
			return this->bodies.size();
		}
		
		Body*
		Model::getBody(const ::std::size_t& i) const
		{
			assert(i < this->bodies.size());
			
			return this->bodies[i];
		}
		
		const ::rl::math::Transform&
		Model::getBodyFrame(const ::std::size_t& i) const
		{
			assert(i < this->getBodies());
			
			return this->bodies[i]->x.transform();
		}
		
		::std::size_t
		Model::getDof() const
		{
			::std::size_t dof = 0;
			
			for (::std::size_t i = 0; i < this->joints.size(); ++i)
			{
				dof += this->joints[i]->getDof();
			}
			
			return dof;
		}
		
		::std::size_t
		Model::getDofPosition() const
		{
			::std::size_t dof = 0;
			
			for (::std::size_t i = 0; i < this->joints.size(); ++i)
			{
				dof += this->joints[i]->getDofPosition();
			}
			
			return dof;
		}
		
		Frame*
		Model::getFrame(const ::std::size_t& i) const
		{
			assert(i < this->frames.size());
			
			return this->frames[i];
		}
		
		::std::size_t
		Model::getFrames() const
		{
			return this->frames.size();
		}
		
		const ::rl::math::Matrix&
		Model::getGammaPosition() const
		{
			return this->gammaPosition;
		}
		
		const ::rl::math::Matrix&
		Model::getGammaVelocity() const
		{
			return this->gammaVelocity;
		}
		
		const ::rl::math::Matrix&
		Model::getGammaPositionInverse() const
		{
			return this->invGammaPosition;
		}
		
		const ::rl::math::Matrix&
		Model::getGammaVelocityInverse() const
		{
			return this->invGammaVelocity;
		}
		
		::rl::math::Vector
		Model::getHomePosition() const
		{
			return this->home;
		}
		
		Joint*
		Model::getJoint(const ::std::size_t& i) const
		{
			assert(i < this->joints.size());
			
			return this->joints[i];
		}
		
		::std::size_t
		Model::getJoints() const
		{
			return this->joints.size();
		}
		
		const ::rl::math::MotionVector&
		Model::getOperationalAcceleration(const ::std::size_t& i) const
		{
			assert(i < this->getOperationalDof());
			
			return this->tree[this->leaves[i]]->a;
		}
		
		::std::size_t
		Model::getOperationalDof() const
		{
			return this->leaves.size();
		}
		
		const ::rl::math::ForceVector&
		Model::getOperationalForce(const ::std::size_t& i) const
		{
			assert(i < this->getOperationalDof());
			
			return this->tree[this->leaves[i]]->f;
		}
		
		Frame*
		Model::getOperationalFrame(const ::std::size_t& i) const
		{
			assert(i < this->getOperationalDof());
			
			return this->tree[this->leaves[i]].get();
		}
		
		const ::rl::math::Transform&
		Model::getOperationalPosition(const ::std::size_t& i) const
		{
			assert(i < this->getOperationalDof());
			
			return this->tree[this->leaves[i]]->x.transform();
		}
		
		const ::rl::math::MotionVector&
		Model::getOperationalVelocity(const ::std::size_t& i) const
		{
			assert(i < this->getOperationalDof());
			
			return this->tree[this->leaves[i]]->v;
		}
		
		const ::std::string&
		Model::getManufacturer() const
		{
			return this->manufacturer;
		}
		
		::rl::math::Vector
		Model::getMaximum() const
		{
			::rl::math::Vector max(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				max.segment(j, this->joints[i]->getDofPosition()) = this->joints[i]->getMaximum();
			}
			
			return max;
		}
		
		::rl::math::Vector
		Model::getMinimum() const
		{
			::rl::math::Vector min(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				min.segment(j, this->joints[i]->getDofPosition()) = this->joints[i]->getMinimum();
			}
			
			return min;
		}
		
		const ::std::string&
		Model::getName() const
		{
			return this->name;
		}
		
		::rl::math::Vector
		Model::getPosition() const
		{
			::rl::math::Vector q(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				q.segment(j, this->joints[i]->getDofPosition()) = this->joints[i]->getPosition();
			}
			
			return this->invGammaPosition * q;
		}
		
		::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1>
		Model::getPositionUnits() const
		{
			::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1> units(this->getDofPosition());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				units.segment(j, this->joints[i]->getDofPosition()) = this->joints[i]->getPositionUnits();
			}
			
			return units;
		}
		
		::rl::math::Vector
		Model::getSpeed() const
		{
			::rl::math::Vector speed(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				speed.segment(j, this->joints[i]->getDof()) = this->joints[i]->getSpeed();
			}
			
			return speed;
		}
		
		::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1>
		Model::getSpeedUnits() const
		{
			::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1> units(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				units.segment(j, this->joints[i]->getDof()) = this->joints[i]->getSpeedUnits();
			}
			
			return units;
		}
		
		::rl::math::Vector
		Model::getTorque() const
		{
			::rl::math::Vector tau(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				tau.segment(j, this->joints[i]->getDof()) = this->joints[i]->getTorque();
			}
			
			return tau;
		}
		
		::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1>
		Model::getTorqueUnits() const
		{
			::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1> units(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				units.segment(j, this->joints[i]->getDof()) = this->joints[i]->getTorqueUnits();
			}
			
			return units;
		}
		
		Transform*
		Model::getTransform(const ::std::size_t& i) const
		{
			assert(i < this->transforms.size());
			
			return this->transforms[i];
		}
		
		::std::size_t
		Model::getTransforms() const
		{
			return this->transforms.size();
		}
		
		::rl::math::Vector
		Model::getVelocity() const
		{
			::rl::math::Vector qd(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				qd.segment(j, this->joints[i]->getDof()) = this->joints[i]->getVelocity();
			}
			
			return this->invGammaVelocity * qd;
		}
		
		::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1>
		Model::getVelocityUnits() const
		{
			::Eigen::Matrix<::rl::math::Units, ::Eigen::Dynamic, 1> units(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				units.segment(j, this->joints[i]->getDof()) = this->joints[i]->getVelocityUnits();
			}
			
			return units;
		}
		
		World*
		Model::getWorld() const
		{
			return dynamic_cast<World*>(this->tree[this->root].get());
		}
		
		const ::rl::math::Vector3&
		Model::getWorldGravity() const
		{
			return dynamic_cast<World*>(this->tree[this->root].get())->getGravity();
		}
		
		::Eigen::Matrix<bool, ::Eigen::Dynamic, 1>
		Model::getWraparounds() const
		{
			::Eigen::Matrix<bool, ::Eigen::Dynamic, 1> wraparounds(this->getDof());
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				wraparounds.segment(j, this->joints[i]->getDof()) = this->joints[i]->wraparound;
			}
			
			return wraparounds;
		}
		
		bool
		Model::isColliding(const ::std::size_t& i) const
		{
			assert(i < this->bodies.size());
			
			return this->bodies[i]->getCollision();
		}
		
		::std::uniform_real_distribution<::rl::math::Real>::result_type
		Model::rand()
		{
			return this->randDistribution(this->randEngine);
		}
		
		void
		Model::remove(Frame* frame)
		{
			::boost::clear_vertex(frame->getVertexDescriptor(), this->tree);
			
			if (dynamic_cast<World*>(frame))
			{
				this->root = 0;
			}
			
			::boost::remove_vertex(frame->getVertexDescriptor(), this->tree);
		}
		
		void
		Model::remove(Transform* transform)
		{
			::boost::remove_edge(transform->getEdgeDescriptor(), this->tree);
		}
		
		void
		Model::seed(const ::std::mt19937::result_type& value)
		{
			this->randEngine.seed(value);
		}
		
		void
		Model::setAcceleration(const ::rl::math::Vector& ydd)
		{
			::rl::math::Vector qdd = this->gammaVelocity * ydd;
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				this->joints[i]->setAcceleration(qdd.segment(j, this->joints[i]->getDof()));
			}
		}
		
		void
		Model::setGammaPosition(const ::rl::math::Matrix& gammaPosition)
		{
			this->gammaPosition = gammaPosition;
			::Eigen::JacobiSVD<::rl::math::Matrix> svd(this->gammaPosition, ::Eigen::ComputeThinU | ::Eigen::ComputeThinV);
			::rl::math::Vector singularValues(svd.singularValues().size());
			
			for (::std::ptrdiff_t i = 0; i < svd.singularValues().size(); ++i)
			{
				singularValues[i] = svd.singularValues()[i] > ::std::numeric_limits<::rl::math::Real>::epsilon() ? 1 / svd.singularValues()[i] : 0;
			}
			
			this->invGammaPosition = svd.matrixV() * singularValues.asDiagonal() * svd.matrixU().transpose();
		}
		
		void
		Model::setGammaVelocity(const ::rl::math::Matrix& gammaVelocity)
		{
			this->gammaVelocity = gammaVelocity;
			::Eigen::JacobiSVD<::rl::math::Matrix> svd(this->gammaVelocity, ::Eigen::ComputeThinU | ::Eigen::ComputeThinV);
			::rl::math::Vector singularValues(svd.singularValues().size());
			
			for (::std::ptrdiff_t i = 0; i < svd.singularValues().size(); ++i)
			{
				singularValues[i] = svd.singularValues()[i] > ::std::numeric_limits<::rl::math::Real>::epsilon() ? 1 / svd.singularValues()[i] : 0;
			}
			
			this->invGammaVelocity = svd.matrixV() * singularValues.asDiagonal() * svd.matrixU().transpose();
		}
		
		void
		Model::setHomePosition(const ::rl::math::Vector& home)
		{
			this->home = home;
		}
		
		void
		Model::setManufacturer(const ::std::string& manufacturer)
		{
			this->manufacturer = manufacturer;
		}
		
		void
		Model::setMaximum(const ::rl::math::Vector& max)
		{
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->setMaximum(max.segment(j, this->joints[i]->getDofPosition()));
			}
		}
		
		void
		Model::setMinimum(const ::rl::math::Vector& min)
		{
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->setMinimum(min.segment(j, this->joints[i]->getDofPosition()));
			}
		}
		
		void
		Model::setName(const ::std::string& name)
		{
			this->name = name;
		}
		
		void
		Model::setOperationalVelocity(const ::std::size_t& i, const ::rl::math::MotionVector& v) const
		{
			this->tree[this->leaves[i]]->v = v;
		}
		
		void
		Model::setPosition(const ::rl::math::Vector& y)
		{
			::rl::math::Vector q = this->gammaPosition * y;
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDofPosition(), ++i)
			{
				this->joints[i]->setPosition(q.segment(j, this->joints[i]->getDofPosition()));
			}
		}
		
		void
		Model::setSpeed(const ::rl::math::Vector& speed)
		{
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				this->joints[i]->setSpeed(speed.segment(j, this->joints[i]->getDof()));
			}
		}
		
		void
		Model::setTorque(const ::rl::math::Vector& tau)
		{
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				this->joints[i]->setTorque(tau.segment(j, this->joints[i]->getDof()));
			}
		}
		
		void
		Model::setVelocity(const ::rl::math::Vector& yd)
		{
			::rl::math::Vector qd = this->gammaVelocity * yd;
			
			for (::std::size_t i = 0, j = 0; i < this->joints.size(); j += this->joints[i]->getDof(), ++i)
			{
				this->joints[i]->setVelocity(qd.segment(j, this->joints[i]->getDof()));
			}
		}
		
		void
		Model::setWorldGravity(const ::rl::math::Vector3& gravity)
		{
			dynamic_cast<World*>(this->tree[this->root].get())->setGravity(gravity);
		}
		
		::rl::math::Transform&
		Model::tool(const ::std::size_t& i)
		{
			assert(i < this->tools.size());
			
			return this->tree[this->tools[i]]->x.transform();
		}
		
		const ::rl::math::Transform&
		Model::tool(const ::std::size_t& i) const
		{
			assert(i < this->tools.size());
			
			return this->tree[this->tools[i]]->x.transform();
		}
		
		void
		Model::update()
		{
			this->bodies.clear();
			this->elements.clear();
			this->joints.clear();
			this->leaves.clear();
			this->tools.clear();
			this->transforms.clear();
			
			this->update(this->root);
		}
		
		void
		Model::update(const Vertex& u)
		{
			Frame* frame = this->tree[u].get();
			this->elements.push_back(frame);
			this->frames.push_back(frame);
			
			if (Body* body = dynamic_cast<Body*>(frame))
			{
				this->bodies.push_back(body);
			}
			
			if (::boost::out_degree(u, this->tree) > 0)
			{
				for (OutEdgeIteratorPair i = ::boost::out_edges(u, this->tree); i.first != i.second; ++i.first)
				{
					Edge e = *i.first;
					Vertex v = ::boost::target(e, this->tree);
					
					Transform* transform = this->tree[e].get();
					this->elements.push_back(transform);
					this->transforms.push_back(transform);
					transform->in = this->tree[u].get();
					transform->out = this->tree[v].get();
					
					if (Joint* joint = dynamic_cast<Joint*>(transform))
					{
						this->joints.push_back(joint);
					}
					
					this->update(v);
				}
			}
			else
			{
				this->leaves.push_back(u);
				
				for (InEdgeIteratorPair i = ::boost::in_edges(u, this->tree); i.first != i.second; ++i.first)
				{
					this->tools.push_back(*i.first);
				}
			}
			
			this->gammaPosition = ::rl::math::Matrix::Identity(this->getDofPosition(), this->getDofPosition());
			this->gammaVelocity = ::rl::math::Matrix::Identity(this->getDof(), this->getDof());
			this->home = ::rl::math::Vector::Zero(this->getDofPosition());
			this->invGammaPosition = ::rl::math::Matrix::Identity(this->getDofPosition(), this->getDofPosition());
			this->invGammaVelocity = ::rl::math::Matrix::Identity(this->getDof(), this->getDof());
		}
		
		::rl::math::Transform&
		Model::world()
		{
			return this->tree[this->root]->x.transform();
		}
		
		const ::rl::math::Transform&
		Model::world() const
		{
			return this->tree[this->root]->x.transform();
		}
	}
}
