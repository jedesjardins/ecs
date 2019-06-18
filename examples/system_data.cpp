
#define JED_TASK_IMPLEMENTATION
#include "task/task.hpp"
#undef JED_TASK_IMPLEMENTATION

#include "ecs/ecs.hpp"

#include <cstdio>


using ComponentMask = uint64_t;
using EntityID = uint64_t;
using Entity = ecs::Entity<EntityID>;
using EntitySpace = ecs::EntitySpace<Entity, ComponentMask>;

struct FrameData
{
	using EntitySpace = ::EntitySpace;

	FrameData(EntitySpace & es, float t)
	:entity_space{es}, dt{t}
	{}

	EntitySpace entity_space;
	float dt;
};

struct PositionComponent
{
	using Manager = ecs::SparseSet<EntityID, PositionComponent>;

	float x;
	float y;

	static size_t get_id()
	{
		return 0;
	}
};

template <typename FrameData>
class SystemData
{
public:
	using EntitySpace = typename FrameData::EntitySpace;
	using Entity = typename EntitySpace::Entity;
	using EntityID = typename Entity::ID;

	SystemData(FrameData & frame_data)
	:m_entity_space{frame_data.entity_space}, dt{frame_data.dt}
	{
	}

	template <typename C>
	C* get_component(Entity const& entity)
	{
		return m_entity_space.template get_component<C>(entity);
	}

	template <typename C>
	bool has_component(Entity const& entity)
	{
		return m_entity_space.template has_component<C>(entity);
	}

	template <typename C>
	void for_each_do(typename ecs::identity<std::function<void (EntityID, C&)>>::type const& func) const
	{
		m_entity_space.template for_each_do<C>(func);
	}

	float const dt;

private:

	EntitySpace const& m_entity_space;
};

struct PositionSystem
{
	using system_data = SystemData<FrameData>;

	static void step(system_data &sd)
	{
		sd.for_each_do<PositionComponent>([&sd](EntityID id, PositionComponent& pos)
		{
			printf("Entity: %llu, Pos: (%f, %f), dt: %f\n", id, pos.x, pos.y, sd.dt);
		});
	}

	static size_t get_id()
	{
		return 0;
	}
};

int main()
{
	tsk::CreateWorkerThreads(1);

	EntitySpace space;

	auto e1 = space.create_entity();

	auto pos_ptr = space.create_component<PositionComponent>(e1, 1.f, 1.f);

	auto e2 = space.create_entity();

	auto pos_ptr2 = space.create_component<PositionComponent>(e2, 0.f, 1.f);

	auto topology = ecs::SystemTopology<FrameData>
	{
		ecs::SystemDependencyList<FrameData>
		{
			ecs::SystemDependency<PositionSystem>{}
		}
	};

	auto fd = FrameData(space, 1000.f/60);

	topology.Step(fd);

	tsk::StopWorkerThreads();
}