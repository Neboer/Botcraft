#pragma once

#include <vector>
#include <memory>
#include <functional>

// A behaviour tree implementation following this blog article
// https://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php
// If you want the (more complete) original post rather than the new one:
// https://web.archive.org/web/20210826210308/https://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php

// Code inspired by (especially for the builder part)
// https://github.com/arvidsson/BrainTree

namespace Botcraft
{
	namespace AI
	{
		enum class Status
		{
			Failure,
			Running,
			Success
		};

		template<class Context>
		class Node
		{
		public:
			Node() {}
			virtual ~Node() {}
			virtual const Status Tick(Context& context) = 0;
		};


		template<class Context>
		class Composite : public Node<Context>
		{
		public:
			Composite() {}
			virtual ~Composite() {}

			void AddChild(std::shared_ptr<Node<Context>> child)
			{
				children.push_back(child);

				// Reset the iterator to the first child
				// every time one child is added
				it = children.begin();
			}

		protected:
			std::vector<std::shared_ptr<Node<Context>>> children;
			class std::vector<std::shared_ptr<Node<Context>>>::iterator it;
		};

		template<class Context>
		class Decorator : public Node<Context>
		{
		public:
			Decorator() { child = nullptr; }
			virtual ~Decorator() {}

			void SetChild(std::shared_ptr<Node<Context>> child_)
			{
				child = child_;
			}

		protected:
			std::shared_ptr<Node<Context>> child;
		};

		template<class Context>
		class Leaf : public Node<Context>
		{
		public:
			Leaf() = delete;
			Leaf(std::function<Status(Context&)> func_)
			{
				func = func_;
			}

			template <class FunctionType, class... Args>
			Leaf(FunctionType func_, Args... args)
			{
				func = [=](Context& c) -> Status { return func_(c, (args)...); };
			}

			virtual ~Leaf() {}
			virtual const Status Tick(Context& context) override
			{
				return func(context);
			}

		private:
			std::function<Status(Context&)> func;
		};

		template<class Context>
		class BehaviourTree : public Node<Context>
		{
		public:
			BehaviourTree() { root = nullptr; }
			virtual ~BehaviourTree() {}

			void SetRoot(const std::shared_ptr<Node<Context>> node) { root = node; }

			virtual const Status Tick(Context& context) override
			{
				if (root == nullptr)
				{
					return Status::Failure;
				}
				return root->Tick(context);
			}

		private:
			std::shared_ptr<Node<Context>> root;
		};

		// Common Composites implementations

		/// <summary>
		/// Sequence implementation. Run all children until
		/// one fails. Succeeds if all children succeed.
		/// Equivalent to a logical AND.
		/// </summary>
		/// <typeparam name="C">The tree context type</typeparam>
		template<class Context>
		class Sequence : public Composite<Context>
		{
		public:
			virtual const Status Tick(Context& context) override
			{
				while (it != children.end())
				{
					Status child_status = (*it)->Tick(context);
					switch (child_status)
					{
					case Status::Failure:
						it = children.begin();
						return child_status;
					case Status::Running:
						return child_status;
					case Status::Success:
						++it;
						break;
					}
				}
				// If we are here, all children succeeded
				it = children.begin();
				return Status::Success;
			}
		};

		/// <summary>
		/// Selector implementation. Run all children until
		/// one succeeds. Fails if all children fail.
		/// Equivalent to a logical OR.
		/// </summary>
		/// <typeparam name="C">The tree context type</typeparam>
		template<class Context>
		class Selector : public Composite<Context>
		{
		public:
			virtual const Status Tick(Context& context) override
			{
				while (it != children.end())
				{
					Status child_status = (*it)->Tick(context);
					switch (child_status)
					{
					case Status::Failure:
						// Move to next child
						++it;
						break;
					case Status::Running:
						return child_status;
					case Status::Success:
						// Reset for next time, return success
						it = children.begin();
						return child_status;
					}
				}
				// If we are here, no child succeeded
				it = children.begin();
				return Status::Failure;
			}
		};



		// Common Decorators implementations

		/// <summary>
		/// A Decorator that inverts the result of its child.
		/// If child returns Running, returns Running too.
		/// </summary>
		/// <typeparam name="C">The tree context type</typeparam>
		template<class Context>
		class Inverter : public Decorator<Context>
		{
		public:
			virtual const Status Tick(Context& context) override
			{
				Status child_status = child->Tick(context);

				switch (child_status)
				{
				case Status::Failure:
					return Status::Success;
				case Status::Running:
					return Status::Running;
				case Status::Success:
					return Status::Failure;
				}
			}
		};

		/// <summary>
		/// A Decorator that always return success,
		/// independently of the result of its child.
		/// Can be combined with an inverter for Failure.
		/// </summary>
		/// <typeparam name="C">The tree context type</typeparam>
		template<class Context>
		class Succeeder : public Decorator<Context>
		{
		public:
			virtual const Status Tick(Context& context) override
			{
				Status child_status = child->Tick(context);

				switch (child_status)
				{
				case Status::Failure:
					return Status::Success;
				case Status::Running:
					return Status::Running;
				case Status::Success:
					return Status::Success;
				}
			}
		};

		/// <summary>
		/// A Decorator that returns success if its child returns success
		/// after max n trials. If its child fails more than n times,
		/// returns failure (run until success if n == 0).
		/// </summary>
		/// <typeparam name="C">The tree context type</typeparam>
		template<class Context>
		class Repeater : public Decorator<Context>
		{
		public:
			Repeater(const size_t n_)
			{
				n = n_;
				conter = 0;
			}

			virtual const Status Tick(Context& context) override
			{
				Status child_status = child->Tick(context);

				switch (child_status)
				{
				case Status::Failure:
					counter++;
					if (counter == n)
					{
						counter = 0;
						return Status::Failure;
					}
					return Status::Running;
				case Status::Running:
					return Status::Running;
				case Status::Success:
					counter = 0;
					return Status::Success;
				}
			}

		private:
			size_t n;
			size_t counter;
		};




		// Builder implementation for easy tree building

		template <class Parent, class Context>
		class DecoratorBuilder;

		template <class Parent, class Context>
		class CompositeBuilder
		{
		public:
			CompositeBuilder(Parent* parent, Composite<Context>* node) : parent(parent), node(node) {}

			// To add a leaf
			template <class... Args>
			CompositeBuilder leaf(Args... args)
			{
				auto child = std::make_shared<Leaf<Context> >((args)...);
				node->AddChild(child);
				return *this;
			}

			// To add a tree
			CompositeBuilder tree(std::shared_ptr<BehaviourTree<Context> > arg)
			{
				node->AddChild(arg);
				return *this;
			}


			// Composite
			// Custom function to add a sequence
			CompositeBuilder<CompositeBuilder, Context> sequence()
			{
				auto child = std::make_shared<Sequence<Context>>();
				node->AddChild(child);
				return CompositeBuilder<CompositeBuilder, Context>(this, (Sequence<Context>*)child.get());
			}

			// Custom function to add a selector
			CompositeBuilder<CompositeBuilder, Context> selector()
			{
				auto child = std::make_shared<Selector<Context>>();
				node->AddChild(child);
				return CompositeBuilder<CompositeBuilder, Context>(this, (Selector<Context>*)child.get());
			}

			// To add any other type of composite
			template <class CompositeType, class... Args>
			CompositeBuilder<CompositeBuilder, Context> composite(Args... args)
			{
				auto child = std::make_shared<CompositeType>((args)...);
				node->AddChild(child);
				return CompositeBuilder<CompositeBuilder, Context>(this, (CompositeType*)child.get());
			}

			// Decorator
			// Inverter
			DecoratorBuilder<CompositeBuilder, Context> inverter()
			{
				auto child = std::make_shared<Inverter<Context>>();
				node->AddChild(child);
				return DecoratorBuilder<CompositeBuilder, Context>(this, (Inverter<Context>*)child.get());
			}

			// Succeeder
			DecoratorBuilder<CompositeBuilder, Context> succeeder()
			{
				auto child = std::make_shared<Succeeder<Context>>();
				node->AddChild(child);
				return DecoratorBuilder<CompositeBuilder, Context>(this, (Succeeder<Context>*)child.get());
			}

			// Repeater
			DecoratorBuilder<CompositeBuilder, Context> repeater(const size_t n)
			{
				auto child = std::make_shared<Repeater<Context>>(n);
				node->AddChild(child);
				return DecoratorBuilder<CompositeBuilder, Context>(this, (Repeater<Context>*)child.get());
			}

			// To add any other type of decorator
			template <class DecoratorType, class... Args>
			DecoratorBuilder<CompositeBuilder, Context> decorator(Args... args)
			{
				auto child = std::make_shared<DecoratorType>((args)...);
				node->AddChild(child);
				return DecoratorBuilder<CompositeBuilder, Context>(this, (DecoratorType*)child.get());
			}

			Parent& end()
			{
				return *parent;
			}

		private:
			Parent* parent;
			Composite<Context>* node;
		};

		template <class Parent, class Context>
		class DecoratorBuilder
		{
		public:
			DecoratorBuilder(Parent* parent, Decorator<Context>* node) : parent(parent), node(node) {}

			// To add a leaf
			template <typename... Args>
			DecoratorBuilder leaf(Args... args)
			{
				auto child = std::make_shared<Leaf<Context> >((args)...);
				node->SetChild(child);
				return *this;
			}

			// To add a tree
			DecoratorBuilder tree(std::shared_ptr<BehaviourTree<Context> > arg)
			{
				node->SetChild(arg);
				return *this;
			}


			// Composites
			// Custom function to add a sequence
			CompositeBuilder<DecoratorBuilder, Context> sequence()
			{
				auto child = std::make_shared<Sequence<Context>>();
				node->AddChild(child);
				return CompositeBuilder<DecoratorBuilder, Context>(this, (Sequence<Context>*)child.get());
			}

			// Custom function to add a selector
			CompositeBuilder<DecoratorBuilder, Context> selector()
			{
				auto child = std::make_shared<Selector<Context>>();
				node->AddChild(child);
				return CompositeBuilder<DecoratorBuilder, Context>(this, (Selector<Context>*)child.get());
			}

			// To add any other type of composite
			template <class CompositeType, class... Args>
			CompositeBuilder<DecoratorBuilder, Context> composite(Args... args)
			{
				auto child = std::make_shared<CompositeType>((args)...);
				node->AddChild(child);
				return CompositeBuilder<DecoratorBuilder, Context>(this, (CompositeType*)child.get());
			}

			// Decorator
			// Inverter
			DecoratorBuilder<DecoratorBuilder, Context> inverter()
			{
				auto child = std::make_shared<Inverter<Context>>();
				node->AddChild(child);
				return DecoratorBuilder<DecoratorBuilder, Context>(this, (Inverter<Context>*)child.get());
			}

			// Succeeder
			DecoratorBuilder<DecoratorBuilder, Context> succeeder()
			{
				auto child = std::make_shared<Succeeder<Context>>();
				node->AddChild(child);
				return DecoratorBuilder<DecoratorBuilder, Context>(this, (Succeeder<Context>*)child.get());
			}

			// Repeater
			DecoratorBuilder<DecoratorBuilder, Context> repeater(const size_t n)
			{
				auto child = std::make_shared<Repeater<Context>>(n);
				node->AddChild(child);
				return DecoratorBuilder<DecoratorBuilder, Context>(this, (Repeater<Context>*)child.get());
			}

			// To add any other type of decorator
			template <class DecoratorType, class... Args>
			DecoratorBuilder<DecoratorBuilder, Context> decorator(Args... args)
			{
				auto child = std::make_shared<DecoratorType>((args)...);
				node->AddChild(child);
				return DecoratorBuilder<DecoratorBuilder, Context>(this, (DecoratorType*)child.get());
			}

			Parent& end()
			{
				return *parent;
			}

		private:
			Parent* parent;
			Decorator<Context>* node;
		};

		// Now that we have CompositeBuilder and DecoratorBuilder
		// we can define the main Builder class
		template<class Context>
		class Builder
		{
		public:
			template <typename... Args>
			Builder leaf(Args... args)
			{
				root = std::make_shared<Leaf<Context> >((args)...);
				return *this;
			}

			// I'm not sure why someone would add a tree as the root of a tree but...
			Builder tree(std::shared_ptr<BehaviourTree<Context> > arg)
			{
				root = arg;
				return *this;
			}


			// Composites
			// Custom function to add a sequence
			CompositeBuilder<Builder, Context> sequence()
			{
				root = std::make_shared<Sequence<Context>>();
				return CompositeBuilder<Builder, Context>(this, (Sequence<Context>*)root.get());
			}

			// Custom function to add a selector
			CompositeBuilder<Builder, Context> selector()
			{
				root = std::make_shared<Selector<Context>>();
				return CompositeBuilder<Builder, Context>(this, (Selector<Context>*)root.get());
			}

			// To add any other type of composite
			template <class CompositeType, class... Args>
			CompositeBuilder<Builder, Context> composite(Args... args)
			{
				root = std::make_shared<CompositeType>((args)...);
				return CompositeBuilder<Builder, Context>(this, (CompositeType*)root.get());
			}

			// Decorator
			// Inverter
			DecoratorBuilder<Builder, Context> inverter()
			{
				root = std::make_shared<Inverter<Context>>(n);
				return DecoratorBuilder<Builder, Context>(this, (Inverter<Context>*)root.get());
			}

			// Succeeder
			DecoratorBuilder<Builder, Context> succeeder()
			{
				root = std::make_shared<Succeeder<Context>>(n);
				return DecoratorBuilder<Builder, Context>(this, (Succeeder<Context>*)root.get());
			}

			// Repeater
			DecoratorBuilder<Builder, Context> repeater(const size_t n)
			{
				root = std::make_shared<Repeater<Context>>(n);
				return DecoratorBuilder<Builder, Context>(this, (Repeater<Context>*)root.get());
			}

			// To add any other type of decorator
			template <class DecoratorType, class... Args>
			DecoratorBuilder<Builder, Context> decorator(Args... args)
			{
				root = std::make_shared<DecoratorType>((args)...);
				return DecoratorBuilder<Builder, Context>(this, (DecoratorType*)root.get());
			}

			std::shared_ptr<BehaviourTree<Context> > build()
			{
				auto tree = std::make_shared<BehaviourTree<Context> >();
				tree->SetRoot(root);
				return tree;
			}

		private:
			std::shared_ptr<Node<Context> > root;
		};


	} // namespace AI
} // namespace Botcraft