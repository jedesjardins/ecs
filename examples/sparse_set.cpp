
#include "ecs/ecs.hpp"

struct Position
{
	Position(float n_x, float n_y)
	:x{n_x}, y{n_y}
	{}

	float x;
	float y;
};

int main()
{
	ecs::SparseSet<int64_t, Position> components;

	components.create(0, 0.f, 0.f);
	components.create(1, 1.f, 1.f);
	components.create(2, 2.f, 2.f);

	components.destroy(1);

	auto pos_ptr0 = components.at(0);
	assert(pos_ptr0 != nullptr);
	assert(pos_ptr0->x == 0.f);

	auto pos_ptr2 = components.at(2);

	printf("%f\n", pos_ptr2->x);
	assert(pos_ptr2 != nullptr);
	assert(pos_ptr2->x == 2.f);

	components.for_each_do([](int64_t id, Position& pos)
	{
		printf("ID: %lld, (x, y): %f, %f\n", id, pos.x, pos.y);
	});


}