
#define JED_TASK_IMPLEMENTATION
#include "task/task.hpp"
#undef JED_TASK_IMPLEMENTATION

#include "ecs/ecs.hpp"

#include <cstdio>


using ComponentMask = uint64_t;
using EntityID = uint64_t;
using Entity = ecs::Entity<EntityID>;
using EntitySpace = ecs::EntitySpace<Entity, ComponentMask>;

struct SD
{
	SD(EntitySpace const& entity_space)
	:m_entity_space{entity_space}
	{
	}

	EntitySpace const& m_entity_space;
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

struct PositionSystem
{
	using system_data = SD;

	static void step(system_data &sd)
	{
		sd.m_entity_space.for_each_do<PositionComponent>([](EntityID id, PositionComponent& pos)
		{
			printf("Entity: %llu, Pos: (%f, %f)\n", id, pos.x, pos.y);
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

	auto topology = ecs::SystemTopology<EntitySpace>
	{
		ecs::SystemDependencyList<EntitySpace>
		{
			ecs::SystemDependency<PositionSystem>{}
		}
	};

	topology.Step(space);

	tsk::StopWorkerThreads();
}