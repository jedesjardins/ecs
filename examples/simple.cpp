
#include <cstdio>

#include "ecs/ecs.hpp"

template <typename C>
class MyComponentManager: public ecs::ComponentManager<MyComponentManager<C>>
{
public:
	template <typename... Args>
	C* create(uint64_t id, Args && ... args)
	{
		C c{std::forward<Args>(args)...};

		auto iter_bool = m_components.emplace(id, c);

		if (iter_bool.second)
		{
			return &iter_bool.first->second;
		}

		return nullptr;
	}

	void destroy(uint64_t id)
	{
		m_components.erase(id);
	}

	C* at(uint64_t id)
	{
		auto const& iter = m_components.find(id);

		if (iter != m_components.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	void for_each_do(typename ecs::identity<std::function<void (uint64_t, C&)>>::type const& func)
	{
		for(auto &iter: m_components)
		{
			func(iter.first, iter.second);
		}
	}

private:
	std::unordered_map<uint64_t, C> m_components;
};



struct Position
{
	using Manager = MyComponentManager<Position>;

	float x;
	float y;

	static size_t get_id()
	{
		return 0;
	}
};

static_assert(std::is_pod<Position>::value, "Position is not pod");
static_assert(std::is_trivial<Position>::value, "Position is not trivial");
static_assert(std::is_trivially_copyable<Position>::value, "Position is not trivially copyable");
static_assert(std::is_standard_layout<Position>::value, "Position is not standard_layout");



int main()
{
	ecs::EntitySpace<ecs::Entity<uint64_t>, uint64_t> space;

	auto e1 = space.create_entity();

	printf("ID: %llu\n", e1.get_id());

	//MyComponentManager<Position> pos_manager;

	//auto pos_ptr = pos_manager.create(e1.get_id(), 1.f, 2.f);

	printf("Here\n");

	auto pos_ptr = space.create_component<Position>(e1, 1.f, 1.f);

	printf("Pos: %f, %f\n", pos_ptr->x, pos_ptr->y);


	printf("Entity has Position: %s\n", space.has_component<Position>(e1) ? "true" : "false");

	auto pos_ptr2 = space.get_component<Position>(e1);

	assert(pos_ptr == pos_ptr2);

	//space.destroy_component<Position>(e1);

	//assert(space.has_component<Position>(e1) == false);

	float inc = 4.f;	
	space.for_each_do<Position>([inc](uint64_t id, Position& pos)
	{
		pos.x += inc;
	});

	printf("\nLooping:\n");
	space.for_each_do<Position>([](uint64_t id, Position& pos)
	{
		printf("Entity %llu: %f, %f\n", id, pos.x, pos.y);
	});
}