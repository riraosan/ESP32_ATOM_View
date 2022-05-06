/*
MIT License

Copyright (c) 2021-2022 riraosan.github.io

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <esp_log.h>
#include <esp32-hal-log.h>
#include <Task.h>

Task::Task(std::string taskName, uint16_t taskSize, uint8_t priority) {
  m_handle   = nullptr;
  m_taskdata = nullptr;
  m_taskname = taskName;
  m_tasksize = taskSize;
  m_priority = priority;
  m_coreid   = tskNO_AFFINITY;
}

Task::~Task() {
}

void Task::runTask(void* pTaskInstance) {
  Task* pTask = (Task*)pTaskInstance;
  log_d(">> Task %s run", pTask->m_taskname.c_str());
  pTask->run(pTask->m_taskdata);
  log_d("<< Task %s stop", pTask->m_taskname.c_str());
  pTask->stop();
}

void Task::start(void* taskData) {
  if (m_handle != nullptr) {
    log_d("Task %s is already running", m_taskname.c_str());
  }
  m_taskdata = taskData;
  ::xTaskCreatePinnedToCore(&runTask, m_taskname.c_str(), m_tasksize, this, m_priority, &m_handle, m_coreid);
}

void Task::stop() {
  if (m_handle == nullptr) {
    return;
  }
  xTaskHandle handleTemp = m_handle;
  m_handle               = nullptr;
  ::vTaskDelete(handleTemp);
}

void Task::delay(int ms) {
  ::vTaskDelay(ms / portTICK_PERIOD_MS);
}

void Task::setTaskSize(uint16_t size) {
  m_tasksize = size;
}

void Task::setTaskPriority(uint8_t priority) {
  m_priority = priority;
}

void Task::setTaskName(std::string name) {
  m_taskname = name;
}

void Task::setCore(BaseType_t coreID) {
  m_coreid = coreID;
}
