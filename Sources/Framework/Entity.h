#pragma once

#include<unordered_map>
#include<typeindex>
#include<memory>

#include"Component.h"

namespace Prizm
{
	class Entity
	{
	private:
		std::unordered_map<std::type_index, std::shared_ptr<Component>> _components;

	public:
		Entity(){}
		virtual ~Entity() = default;

		virtual bool Initialize(void) = 0;
		virtual void Run(void) = 0;
		virtual void Draw(void) = 0;
		virtual void Finalize(void) = 0;

		template<typename _ComTy, typename ... Args>
		void AddComponent(const Args& ... args)
		{
			auto component = _components[typeid(_ComTy)] = std::make_shared<_ComTy>(args ...);
			component->SetOwner(this);
			component->Initialize();
		}

		template<typename _ComTy>
		std::shared_ptr<Component>& GetComponent(void)
		{
			return std::static_pointer_cast<_ComTy>(_components[typeid(_ComTy)]);
		}

		void RunComponets(void);
		void DrawComponents(void);
		void FinalizeComponets(void);
	};
}