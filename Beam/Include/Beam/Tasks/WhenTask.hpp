#ifndef BEAM_WHENTASK_HPP
#define BEAM_WHENTASK_HPP
#include "Beam/Pointers/Ref.hpp"
#include "Beam/Reactors/DoReactor.hpp"
#include "Beam/Reactors/QueueReactor.hpp"
#include "Beam/Reactors/ReactorMonitor.hpp"
#include "Beam/Reactors/WhenComplete.hpp"
#include "Beam/SignalHandling/ScopedSlotAdaptor.hpp"
#include "Beam/Tasks/BasicTask.hpp"
#include "Beam/Tasks/Tasks.hpp"

namespace Beam {
namespace Tasks {

  /*! \class WhenTask
      \brief Executes a Task when a condition is triggered.
   */
  class WhenTask : public BasicTask {
    public:

      //! Constructs a WhenTask.
      /*!
        \param taskFactory The Task to execute when the condition is
               <code>true</code>.
        \param condition The condition that executes the Task.
        \param reactorMonitor The ReactorMonitor to use.
      */
      WhenTask(TaskFactory taskFactory,
        std::shared_ptr<Reactors::Reactor<bool>> condition,
        RefType<Reactors::ReactorMonitor> reactorMonitor);

    protected:
      virtual void OnExecute() override final;

      virtual void OnCancel() override final;

    private:
      TaskFactory m_taskFactory;
      std::shared_ptr<Reactors::Reactor<bool>> m_condition;
      Reactors::ReactorMonitor* m_reactorMonitor;
      int m_state;
      SignalHandling::ScopedSlotAdaptor m_callbacks;

      void OnCondition(const Expect<bool>& condition);
      void OnConditionComplete();
      void OnTaskUpdate(const StateEntry& update);
      void S0();
      void S1(State state, const std::string& message);
      void S2(const std::string& message);
      void S3();
      void S4();
  };

  /*! \class WhenTaskFactory
      \brief Implements the TaskFactory for WhenTasks.
   */
  class WhenTaskFactory : public BasicTaskFactory<WhenTaskFactory> {
    public:

      //! Constructs a WhenTaskFactory.
      /*!
        \param taskFactory The Task to execute when the condition is
               <code>true</code>.
        \param condition The condition that executes the Task.
        \param reactorMonitor The ReactorMonitor to use.
      */
      WhenTaskFactory(TaskFactory taskFactory,
        std::shared_ptr<Reactors::Reactor<bool>> condition,
        RefType<Reactors::ReactorMonitor> reactorMonitor);

      virtual std::shared_ptr<Task> Create() override final;

    private:
      TaskFactory m_taskFactory;
      std::shared_ptr<Reactors::Reactor<bool>> m_condition;
      Reactors::ReactorMonitor* m_reactorMonitor;
  };

  inline WhenTask::WhenTask(TaskFactory taskFactory,
      std::shared_ptr<Reactors::Reactor<bool>> condition,
      RefType<Reactors::ReactorMonitor> reactorMonitor)
      : m_taskFactory{std::move(taskFactory)},
        m_condition{std::move(condition)},
        m_reactorMonitor{reactorMonitor.Get()} {}

  inline void WhenTask::OnExecute() {
    return S0();
  }

  inline void WhenTask::OnCancel() {
    m_reactorMonitor->Do(
      [=] {
        if(m_state == 0) {
          return S1(State::CANCELED, "");
        } else if(m_state == 4) {
          return S1(State::CANCELED, "");
        }
      });
  }

  inline void WhenTask::OnCondition(const Expect<bool>& condition) {
    if(m_state == 0) {
      try {
        if(condition.Get()) {
          return S3();
        }
      } catch(const std::exception& e) {
        return S2(e.what());
      }
    }
  }

  inline void WhenTask::OnConditionComplete() {
    if(m_state == 0) {
      return S1(State::COMPLETE, "");
    }
  }

  inline void WhenTask::OnTaskUpdate(const StateEntry& update) {
    if(m_state == 4) {
      if(update.m_state == State::FAILED) {
        return S2(update.m_message);
      } else if(IsTerminal(update.m_state)) {
        return S1(update.m_state, update.m_message);
      }
    }
  }

  inline void WhenTask::S0() {
    m_state = 0;
    SetActive();
    auto reactor = Reactors::WhenComplete(
      std::bind(&WhenTask::OnConditionComplete, this),
      Reactors::Do(m_callbacks.GetCallback(
      std::bind(&WhenTask::OnCondition, this, std::placeholders::_1)),
      m_condition));
    m_reactorMonitor->Add(reactor);
  }

  inline void WhenTask::S1(State state, const std::string& message) {
    m_state = 1;
    SetTerminal(state, message);
  }

  inline void WhenTask::S2(const std::string& message) {
    m_state = 2;
    SetTerminal(State::FAILED, message);
  }

  inline void WhenTask::S3() {
    m_state = 3;
    auto task = m_taskFactory->Create();
    Manage(task);
    auto publisher = Reactors::MakePublisherReactor(task->GetPublisher(),
      Ref(m_reactorMonitor->GetTrigger()));
    auto taskReactor = Reactors::Do(m_callbacks.GetCallback(
      std::bind(&WhenTask::OnTaskUpdate, this, std::placeholders::_1)),
      publisher);
    m_reactorMonitor->Add(taskReactor);
    task->Execute();
    return S4();
  }

  inline void WhenTask::S4() {
    m_state = 4;
  }

  inline WhenTaskFactory::WhenTaskFactory(TaskFactory taskFactory,
      std::shared_ptr<Reactors::Reactor<bool>> condition,
      RefType<Reactors::ReactorMonitor> reactorMonitor)
      : m_taskFactory{std::move(taskFactory)},
        m_condition{std::move(condition)},
        m_reactorMonitor{reactorMonitor.Get()} {}

  inline std::shared_ptr<Task> WhenTaskFactory::Create() {
    return std::make_shared<WhenTask>(m_taskFactory, m_condition,
      Ref(*m_reactorMonitor));
  }
}
}

#endif
