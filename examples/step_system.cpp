
#define JED_TASK_IMPLEMENTATION
#include "task/task.hpp"
#undef JED_TASK_IMPLEMENTATION

#include "ecs/ecs.hpp"

#include <cstdio>

struct PrintSystem
{
	struct system_data
	{
		system_data(int &fd)
		:m_fd{fd}
		{
		}

		void print()
		{
			printf("%d\n", m_fd);
		}

		int &m_fd;
	};

	static void step(system_data &sd)
	{
		printf("PositionSystem, data: %d\n", sd.m_fd);
		sd.m_fd = 1;
	}
};


int main()
{
	int fd{0};

	auto func = &ecs::step_system<PrintSystem, int>;

	func(fd);

	assert(fd == 1);
}