// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "backend/opengl/QueueGL.h"

#include "backend/opengl/CommandBufferGL.h"
#include "backend/opengl/DeviceGL.h"

namespace backend { namespace opengl {

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    void Queue::Submit(uint32_t numCommands, CommandBuffer* const* commands) {
        for (uint32_t i = 0; i < numCommands; ++i) {
            commands[i]->Execute();
        }
    }

}}  // namespace backend::opengl
