

#define JED_TASK_IMPLEMENTATION
#include "task/task.hpp"
#undef JED_TASK_IMPLEMENTATION

#include "ecs/ecs.hpp"

#include <cstdio>

struct SD
{
	SD(int &fd)
	:m_fd{fd}
	{
	}

	void print()
	{
		printf("%d\n", m_fd);
	}

	int &m_fd;
};

struct FirstSystem
{
	using system_data = SD;

	static void step(system_data &sd)
	{
		printf("In First\n");
		assert(sd.m_fd == 0 && "FirstSystem ran first");
		sd.m_fd = 1;
	}

	static size_t get_id()
	{
		return 0;
	}
};

struct SecondSystem
{
	using system_data = SD;

	static void step(system_data &sd)
	{
		printf("In Second\n");
		assert(sd.m_fd == 1 && "SecondSystem ran second");
		sd.m_fd = 2;
	}

	static size_t get_id()
	{
		return 1;
	}
};

struct ThirdSystem
{
	using system_data = SD;

	static void step(system_data &sd)
	{
		printf("In Third\n");
		assert(sd.m_fd == 2 && "ThirdSystem ran third");
		sd.m_fd = 3;
	}

	static size_t get_id()
	{
		return 2;
	}
};

struct UnrelatedSystem
{
	using system_data = SD;

	static void step(system_data &sd)
	{
		printf("In Unrelated\n");
	}

	static size_t get_id()
	{
		return 3;
	}
};

int main()
{
	tsk::CreateWorkerThreads(1);

	auto topology = ecs::SystemTopology<int>
	{
		ecs::SystemDependencyList<int>
		{
			ecs::SystemDependency<FirstSystem>{},
			ecs::SystemDependency<SecondSystem, FirstSystem>{},
			ecs::SystemDependency<ThirdSystem, SecondSystem>{},
			ecs::SystemDependency<UnrelatedSystem>{}
		}
	};

	int x = 0;

	topology.Step(x);

	assert(x == 3 && "Systems didn't run correctly");

	tsk::StopWorkerThreads();
}