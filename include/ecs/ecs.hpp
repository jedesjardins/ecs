#ifndef JED_ECS_HPP
#define JED_ECS_HPP

#include "task/task.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

#include <set>
#include <deque>
#include <string>

#include <chrono>
#include <thread>

namespace ecs
{

template <typename T>
struct identity
{
	typedef T type;
}; // class identity



class BaseComponentManager
{}; // class BaseComponentManager

template <typename Impl>
class ComponentManager: public BaseComponentManager
{
	// run static asserts for functions here
}; // class ComponentManager

template <typename ID, typename C>
class SparseSet: public BaseComponentManager
{
public:
	template <typename... Args>
	C* create(ID id, Args && ... args)
	{
		size_t new_index = m_components.size();

		m_components.emplace_back(new_index, C{std::forward<Args>(args)...});

		m_component_lookup[id] = new_index;

		return &m_components.back().second;
	}

	void destroy(ID id)
	{
		//m_components.erase(id);

		// find the location in the contiguous list

		auto lookup_iter = m_component_lookup.find(id);
		if (lookup_iter == m_component_lookup.end())
		{
			return;
		}

		size_t remove_location = lookup_iter->second;

		// swap the back with the erased element and erase it
		std::swap(m_components.at(remove_location), m_components.back());

		m_components.pop_back();

		// update the lookup map of the swapped component
		auto moved_iter = m_component_lookup.find(m_components.at(remove_location).first);
		if (moved_iter == m_component_lookup.end())
		{
			// what? Nothing to update..?
			printf("Huh\n");
			return;
		}

		moved_iter->second = remove_location;
	}

	C* at(ID id)
	{
		auto const& iter = m_component_lookup.find(id);

		if (iter == m_component_lookup.end())
		{
			return nullptr;
		}

		return &m_components[iter->second].second;		
	}

	void for_each_do(typename identity<std::function<void (ID, C&)>>::type const& func)
	{
		for(auto &iter: m_components)
		{
			func(iter.first, iter.second);
		}
	}

private:
	std::unordered_map<ID, size_t> m_component_lookup;
	std::vector<std::pair<ID, C>> m_components;

}; // class SparseSet



template <typename T>
class Entity
{
public:
	using ID = T;

	Entity(ID const& id)
	:m_id{id}
	{}

	ID get_id() const
	{
		return m_id;
	}

private:
	ID m_id{};

}; // class Entity



template <typename E, typename ComponentMask>
class EntitySpace
{
public:
	using Entity = E;
	using EntityID = typename Entity::ID;

	// create entity
	Entity create_entity()
	{
		EntityID id{get_next_id()};

		m_component_masks.emplace(id, ComponentMask{});

		return Entity{id};
	}

	// destroy entity
	void destroy_entity(Entity const& entity)
	{
		auto entity_id = entity.get_id();

		//TODO: assert the entity exists

		// remove all the components
		auto iter = m_component_masks.find(entity_id);

		if (iter != m_component_masks.end())
		{
			auto entity_mask = iter.second();

			for (int i = 0; i < 64; ++i)
			{
				if (entity_mask & (1 << i))
				{
					m_component_managers[i]->destroy(entity_id);
				}
			}
		}

		// remove entity mask and mark the id for reuse
		m_component_masks.erase(iter);
		release_id(entity_id);
	}

	// create component
	template <typename C, typename... Args>
	C* create_component(Entity const& entity, Args && ... args)
	{
		using CManager = typename C::Manager;

		// get the manager (or create it)
		auto entity_id = entity.get_id();
		auto component_id = C::get_id();
		CManager * casted_component_manager = nullptr;

		if (m_component_managers.size() <= component_id)
		{
			m_component_managers.resize(component_id+1);
		}

		auto &component_manager = m_component_managers.at(component_id);
		if (!component_manager)
		{
			casted_component_manager = new CManager();
			component_manager = casted_component_manager;
		}
		else
		{
			casted_component_manager = static_cast<CManager*>(component_manager);
		}


		// forward the arguments on to the manager and create the component


		C * c_ptr = casted_component_manager->create(entity.get_id(), std::forward<Args>(args)...);


		// add to the entity mask
		auto mask_iter = m_component_masks.find(entity.get_id());
		if (mask_iter == m_component_masks.end())
		{
			return nullptr;
		}

		ComponentMask & entity_mask = mask_iter->second;
		entity_mask |= 1 << component_id;
		
		return c_ptr;
	}

	// destroy component
	template <typename C>
	void destroy_component(Entity const& entity)
	{
		using CManager = typename C::Manager;

		auto component_id = C::get_id();

		// remove from the entity mask
		auto mask_iter = m_component_masks.find(entity.get_id());
		if (mask_iter != m_component_masks.end())
		{
			ComponentMask & entity_mask = mask_iter->second;
			entity_mask &= ~(1 << component_id);
		}

		// destroy in the manager
		if (m_component_managers.size() >= component_id)
		{
			auto component_manager = m_component_managers.at(component_id);

			// destroy the component
			if (component_manager)
			{
				auto casted_component_manager = static_cast<CManager*>(component_manager);
				casted_component_manager->destroy(entity.get_id());
			}
		}
	}

	// get component
	template <typename C>
	C* get_component(Entity const& entity) const
	{
		using CManager = typename C::Manager;

		auto component_id = C::get_id();
		if (m_component_managers.size() < component_id)
		{
			return nullptr;
		}

		// find the manager
		auto component_manager = m_component_managers.at(component_id);
		if (!component_manager)
		{
			return nullptr;
		}

		auto casted_component_manager = static_cast<CManager*>(component_manager);
		return casted_component_manager->at(entity.get_id());
	}

	// has component
	template <typename C>
	bool has_component(Entity const& entity) const
	{
		// check the mask?
		//auto &entity_mask = m_component_masks.at(entity.get_id());

		auto mask_iter = m_component_masks.find(entity.get_id());
		if (mask_iter == m_component_masks.end())
		{
			return false;
		}
		
		ComponentMask const& entity_mask = mask_iter->second;
		return entity_mask & (1 << C::get_id());
	}

	template <typename C>
	void for_each_do(typename identity<std::function<void (EntityID, C&)>>::type const& func) const
	{
		using CManager = typename C::Manager;

		auto component_id = C::get_id();
		if (m_component_managers.size() < component_id)
		{
			return;
		}

		auto &component_manager = m_component_managers.at(component_id);
		if (component_manager)
		{
			auto casted_component_manager = static_cast<CManager*>(component_manager);
			casted_component_manager->for_each_do(func);
			return;
		}
	}

private:
	EntityID get_next_id()
	{
		return next_id++;
	}

	void release_id(EntityID const&)
	{
	}

	std::unordered_map<EntityID, ComponentMask> m_component_masks{};
	std::vector<BaseComponentManager*> m_component_managers{};
	EntityID next_id{};

}; // class EntitySpace






// unnecessary??
template <typename System>
struct BaseSystem
{
	template <typename frame_data>
	static void step_system(frame_data & fd)
	{
		typename System::system_data sd{fd};

		System::step(sd);
	}
}; // class BaseSystem

template <typename System, typename frame_data>
void step_system(frame_data & fd)
{
	typename System::system_data sd{fd};

	System::step(sd);
}

template <typename FD>
using SystemProcessFunc = void (*)(FD &);

/*
template <typename SD>
class DerivedSystem: public BaseSystem<DerivedSystem<SD>>
{
public:
	using system_data = SD;

	static void step(system_data & sd)
	{
		// use sd here
	}
};

//usage
using step_wrapper = void (*)(int &);

step_wrapper derived_system_step = DerivedSystem<int>::step_system<int>;

int x = 1;
derived_system_step(1);
*/

using SystemID = size_t;

template <typename S, typename... Others>
struct SystemDependency
{
	using type = S;
	using others = std::tuple<Others...>;
};

template <typename FD>
class SystemDependencyList
{
public:
	template <typename... Ds>
	SystemDependencyList(Ds... args)
	:system_dependencies{},
	 system_func_ptrs{}
	{
		AddChild<Ds...>();
	}

	template <typename DL>
	void AddChild()
	{
		using SystemType = typename DL::type;

		// add D::type process function to a list
		system_func_ptrs[SystemType::get_id()] = &step_system<SystemType, FD>;

		AddDependencies
		(
			system_dependencies[SystemType::get_id()],
			typename DL::others{}
		);
	}

	template <typename DL1, typename DL2, typename... DLS>
	void AddChild()
	{
		AddChild<DL1>();
		AddChild<DL2, DLS...>();
	}

	template <typename... Others>
	void AddDependencies(std::set<SystemID> &dependencies, std::tuple<Others...> const&)
	{
		AddDependency<Others...>(dependencies);
	}

	template <>
	void AddDependencies(std::set<SystemID> &dependencies, std::tuple<> const&)
	{}

	template <typename D>
	void AddDependency(std::set<SystemID> &dependencies)
	{
		// add D process function to list
		system_func_ptrs[D::get_id()] = &step_system<D, FD>;

		dependencies.insert(D::get_id());
	}

private:
	template <typename T>
	friend class SystemTopology;

	std::unordered_map<SystemID, std::set<SystemID>> system_dependencies;
	std::unordered_map<SystemID, ecs::SystemProcessFunc<FD>> system_func_ptrs;

};





template <typename FD>
class SystemTopology
{
public:

	// this constructor is used by lua so pass it lua to call any systems that aren't c++
	/*
	SystemTopology(std::unordered_map<std::string, std::set<std::string>> const& system_dependencies,
		std::unordered_map<std::string, SystemID> const& system_ids,
		std::unordered_map<SystemID, ecs::SystemProcessFunc> const& system_ids_to_ptrs,
		bool print_output = false)
	{

		std::unordered_map<SystemID, SystemNode> systems;

		// create a SystemNode for each system and it's dependencies
		for (auto const& system_dependency: system_dependencies)
		{
			auto system_name = system_dependency.first;
			auto &dependency_list = system_dependency.second;

			assert(system_ids.find(system_name) != system_ids.cend());

			auto system_id = system_ids.at(system_name);

			systems[system_id];

			if (system_tasks.find(system_id) == system_tasks.end())
			{
				system_tasks[system_id] = {system_ids_to_ptrs.at(system_id), tsk::NULL_TASK, system_name};
			}
			else
			{
				system_tasks[system_id] = {&ecs::ProcessLuaSystem, tsk::NULL_TASK, system_name};
			}

			for (auto &dependency_name: dependency_list)
			{
				assert(system_name != dependency_name);
				assert(system_ids.find(dependency_name) != system_ids.cend());

				auto dependency_id = system_ids.at(dependency_name);

				systems[system_id].parents.insert(dependency_id);
				systems[dependency_id].children.insert(system_id);

				if (system_tasks.find(dependency_id) == system_tasks.end())
				{
					system_tasks[dependency_id] = {system_ids_to_ptrs.at(dependency_id), tsk::NULL_TASK, dependency_name};
				}
				else
				{
					system_tasks[system_id] = {&ecs::ProcessLuaSystem, tsk::NULL_TASK, dependency_name};
				}
			}
		}

		InitFromSystemsMap(systems);
	}
	*/

	SystemTopology(SystemDependencyList<FD> const& list)
	:system_tasks{}, clusters{}, cluster_order{}
	{
		auto const& system_dependencies = list.system_dependencies;
		auto const& system_ids_to_ptrs = list.system_func_ptrs;

		std::unordered_map<SystemID, SystemNode> systems;

		for (auto const& system_dependency: system_dependencies)
		{
			auto system_id = system_dependency.first;

			systems[system_id];

			if (system_tasks.find(system_id) == system_tasks.end())
			{
				assert(system_ids_to_ptrs.find(system_id) != system_ids_to_ptrs.cend());
				system_tasks[system_id] = {system_ids_to_ptrs.at(system_id), tsk::NULL_TASK, ""};
			}

			auto dependency_list = system_dependency.second;

			for (auto &dependency_id: dependency_list)
			{
				systems[system_id].parents.insert(dependency_id);
				systems[dependency_id].children.insert(system_id);

				if (system_tasks.find(dependency_id) == system_tasks.end())
				{
					system_tasks[dependency_id] = {system_ids_to_ptrs.at(dependency_id), tsk::NULL_TASK, ""};
				}
			}
		}

		InitFromSystemsMap(systems);
	}

	void Step(FD & fd)
	{
		// empty task to track children
		auto parent_task = tsk::CreateTask([](size_t thread_id, tsk::TaskId this_task, void* task_data){});

		for (auto cluster_id: cluster_order)
		{
			// build continuation list
			auto & cluster = clusters[cluster_id];

			tsk::TaskId prev_task{tsk::NULL_TASK};
			tsk::TaskId this_task{tsk::NULL_TASK};
			tsk::TaskId root_task{tsk::NULL_TASK};

			for (auto system_id: cluster.system_id_list)
			{


				this_task = tsk::CreateTask([](size_t thread_id, tsk::TaskId this_task, void* task_data)
				{
					TaskData * td = tsk::GetData<TaskData *>(task_data);
					td->system_ptr(*td->frame_data);
					delete td;
				});

				auto const& system_task = system_tasks[system_id];


				// how should I free this??
				tsk::StoreData(this_task, new TaskData{system_task.system_ptr, &fd});				
				tsk::SetParent(this_task, parent_task);
				

				if (prev_task != tsk::NULL_TASK)
				{
					tsk::AddContinuation(prev_task, this_task);
				}
				else
				{
					root_task = this_task;
				}

				system_tasks[system_id].task_id = this_task;

				prev_task = this_task;
			}

			for (auto system_id: cluster.system_dependencies)
			{
				tsk::Wait(system_tasks[system_id].task_id);
			}

			tsk::AsyncRun(root_task);
		}

		tsk::AsyncRun(parent_task);
		tsk::Wait(parent_task);
	}

private:

	using ContinuationId = int;
	using ClusterId = size_t;

	struct SystemNode
	{
		ContinuationId continuation_id{-1};
		std::set<SystemID> parents;
		std::set<SystemID> children;
	};

	struct Cluster;

	struct Continuation
	{
		std::set<ContinuationId> parents;
		std::set<ContinuationId> children;

		ClusterId cluster_id;
	};

	struct SystemTask
	{
		ecs::SystemProcessFunc<FD> system_ptr;
		tsk::TaskId task_id;
		std::string name;
	};

	struct Cluster
	{
		std::set<SystemID> system_dependencies;
		std::vector<SystemID> system_id_list;
	};

	/* this is now FD, frame_data
	struct TaskData
	{
		ecs::SystemProcessFunc<FD> system_ptr{nullptr};
		sol::state * lua{nullptr};
		char const* system_name{nullptr};
		ecs::EntitySpace * space_ptr{nullptr};
	};
	*/

	struct TaskData
	{
		ecs::SystemProcessFunc<FD> system_ptr{nullptr};
		FD * frame_data{nullptr};
	};

	// INIT
	void InitFromSystemsMap(std::unordered_map<SystemID, SystemNode> & systems)
	{
		// TODO: check for cycles


		ContinuationId new_cont_id{0};
		std::vector<Continuation> continuations;

		// sort systems into their continuations
		for (auto &system_iter: systems)
		{
			auto system_id = system_iter.first;
			auto &system_node = system_iter.second;

			// this system is a root of a continuation
			if (system_node.parents.size() != 1)
			{

				// create new continuation & cluster
				auto cont_id = new_cont_id++;

				continuations.emplace_back();
				auto &cont = continuations.back();

				clusters.emplace_back();
				auto &cont_cluster = clusters.back();

				cont.cluster_id = clusters.size()-1;

				// mark this system as being in the new continuation
				system_node.continuation_id = cont_id;

				// add this systems dependencies as dependencies for the continuation
				for (auto &ancestor_system_id: system_node.parents)
				{
					cont_cluster.system_dependencies.insert(ancestor_system_id);
				}

				// add this system to the list of systems in the continuation
				cont_cluster.system_id_list.push_back(system_id);

				// add all children here?
				std::vector<SystemID> descendent_ids(system_node.children.begin(), system_node.children.end());

				while (descendent_ids.size())
				{
					auto &next_system_id = descendent_ids.back();
					descendent_ids.pop_back();

					auto &next_system_node = systems[next_system_id];

					if (next_system_node.continuation_id == -1 && next_system_node.parents.size() == 1)
					{
						next_system_node.continuation_id = cont_id;
						cont_cluster.system_id_list.push_back(next_system_id);
						descendent_ids.insert(descendent_ids.end(), next_system_node.children.begin(), next_system_node.children.end());
					}
				}
			}
		}



		std::deque<ContinuationId> frontier;

		for (ContinuationId cont_id = 0; cont_id < continuations.size(); ++cont_id)
		{
			auto &cont = continuations[cont_id];
			auto &cont_cluster = clusters[cont.cluster_id];

			if (!cont_cluster.system_dependencies.size())
			{
				frontier.push_back(cont_id);
			}

			for (auto dependency_system_id: cont_cluster.system_dependencies)
			{
				auto &dependency_system = systems[dependency_system_id];

				if (cont.parents.insert(dependency_system.continuation_id).second)
				{
					continuations[dependency_system.continuation_id].children.insert(cont_id);
				}
			}
		}

		std::unordered_map<ContinuationId, bool> finished_cont_ids;

		while (frontier.size())
		{
			auto cont_id = frontier.front();
			frontier.pop_front();
			auto &cont = continuations[cont_id];

			if (!finished_cont_ids[cont_id])
			{
				// are all of it's parents finished?
				bool parents_are_finished = true;
				for (auto parent_id: cont.parents)
				{
					if (!finished_cont_ids[parent_id])
					{
						parents_are_finished = false;
					}
				}

				// if parents aren't finished skip
				if (!parents_are_finished)
				{
					continue;
				}

				for (auto child_id: cont.children)
				{
					frontier.push_back(child_id);
				}

				finished_cont_ids[cont_id] = true;
				//cluster_order.push_back(cont_id);
				cluster_order.push_back(cont.cluster_id);
			}
		}
	}

	std::unordered_map<SystemID, SystemTask> system_tasks;
	std::vector<Cluster> clusters;
	std::vector<ClusterId> cluster_order;

}; // class SystemTopology

}; // namespace ecs

#endif