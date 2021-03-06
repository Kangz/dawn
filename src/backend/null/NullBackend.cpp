// Copyright 2017 The Dawn Authors
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

#include "backend/null/NullBackend.h"

#include "backend/Commands.h"

#include <spirv-cross/spirv_cross.hpp>

namespace backend { namespace null {

    dawnProcTable GetNonValidatingProcs();
    dawnProcTable GetValidatingProcs();

    void Init(dawnProcTable* procs, dawnDevice* device) {
        *procs = GetValidatingProcs();
        *device = reinterpret_cast<dawnDevice>(new Device);
    }

    // Device

    Device::Device() {
    }

    Device::~Device() {
    }

    BindGroupBase* Device::CreateBindGroup(BindGroupBuilder* builder) {
        return new BindGroup(builder);
    }
    ResultOrError<BindGroupLayoutBase*> Device::CreateBindGroupLayoutImpl(
        const dawn::BindGroupLayoutDescriptor* descriptor) {
        return new BindGroupLayout(this, descriptor);
    }
    BlendStateBase* Device::CreateBlendState(BlendStateBuilder* builder) {
        return new BlendState(builder);
    }
    BufferBase* Device::CreateBuffer(BufferBuilder* builder) {
        return new Buffer(builder);
    }
    BufferViewBase* Device::CreateBufferView(BufferViewBuilder* builder) {
        return new BufferView(builder);
    }
    CommandBufferBase* Device::CreateCommandBuffer(CommandBufferBuilder* builder) {
        return new CommandBuffer(builder);
    }
    ComputePipelineBase* Device::CreateComputePipeline(ComputePipelineBuilder* builder) {
        return new ComputePipeline(builder);
    }
    DepthStencilStateBase* Device::CreateDepthStencilState(DepthStencilStateBuilder* builder) {
        return new DepthStencilState(builder);
    }
    InputStateBase* Device::CreateInputState(InputStateBuilder* builder) {
        return new InputState(builder);
    }
    ResultOrError<PipelineLayoutBase*> Device::CreatePipelineLayoutImpl(
        const dawn::PipelineLayoutDescriptor* descriptor) {
        return new PipelineLayout(this, descriptor);
    }
    ResultOrError<QueueBase*> Device::CreateQueueImpl() {
        return new Queue(this);
    }
    RenderPassDescriptorBase* Device::CreateRenderPassDescriptor(
        RenderPassDescriptorBuilder* builder) {
        return new RenderPassDescriptor(builder);
    }
    RenderPipelineBase* Device::CreateRenderPipeline(RenderPipelineBuilder* builder) {
        return new RenderPipeline(builder);
    }
    ResultOrError<SamplerBase*> Device::CreateSamplerImpl(
        const dawn::SamplerDescriptor* descriptor) {
        return new Sampler(this, descriptor);
    }
    ShaderModuleBase* Device::CreateShaderModule(ShaderModuleBuilder* builder) {
        auto module = new ShaderModule(builder);

        spirv_cross::Compiler compiler(builder->AcquireSpirv());
        module->ExtractSpirvInfo(compiler);

        return module;
    }
    SwapChainBase* Device::CreateSwapChain(SwapChainBuilder* builder) {
        return new SwapChain(builder);
    }
    TextureBase* Device::CreateTexture(TextureBuilder* builder) {
        return new Texture(builder);
    }
    TextureViewBase* Device::CreateTextureView(TextureViewBuilder* builder) {
        return new TextureView(builder);
    }

    void Device::TickImpl() {
    }

    void Device::AddPendingOperation(std::unique_ptr<PendingOperation> operation) {
        mPendingOperations.emplace_back(std::move(operation));
    }
    std::vector<std::unique_ptr<PendingOperation>> Device::AcquirePendingOperations() {
        return std::move(mPendingOperations);
    }

    // Buffer

    struct BufferMapReadOperation : PendingOperation {
        virtual void Execute() {
            buffer->MapReadOperationCompleted(serial, ptr, isWrite);
        }

        Ref<Buffer> buffer;
        void* ptr;
        uint32_t serial;
        bool isWrite;
    };

    Buffer::Buffer(BufferBuilder* builder) : BufferBase(builder) {
        if (GetAllowedUsage() & (dawn::BufferUsageBit::TransferDst | dawn::BufferUsageBit::MapRead |
                                 dawn::BufferUsageBit::MapWrite)) {
            mBackingData = std::unique_ptr<char[]>(new char[GetSize()]);
        }
    }

    Buffer::~Buffer() {
    }

    void Buffer::MapReadOperationCompleted(uint32_t serial, void* ptr, bool isWrite) {
        if (isWrite) {
            CallMapWriteCallback(serial, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, ptr);
        } else {
            CallMapReadCallback(serial, DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS, ptr);
        }
    }

    void Buffer::SetSubDataImpl(uint32_t start, uint32_t count, const uint8_t* data) {
        ASSERT(start + count <= GetSize());
        ASSERT(mBackingData);
        memcpy(mBackingData.get() + start, data, count);
    }

    void Buffer::MapReadAsyncImpl(uint32_t serial, uint32_t start, uint32_t count) {
        MapAsyncImplCommon(serial, start, count, false);
    }

    void Buffer::MapWriteAsyncImpl(uint32_t serial, uint32_t start, uint32_t count) {
        MapAsyncImplCommon(serial, start, count, true);
    }

    void Buffer::MapAsyncImplCommon(uint32_t serial, uint32_t start, uint32_t count, bool isWrite) {
        ASSERT(start + count <= GetSize());
        ASSERT(mBackingData);

        auto operation = new BufferMapReadOperation;
        operation->buffer = this;
        operation->ptr = mBackingData.get() + start;
        operation->serial = serial;
        operation->isWrite = isWrite;

        ToBackend(GetDevice())->AddPendingOperation(std::unique_ptr<PendingOperation>(operation));
    }

    void Buffer::UnmapImpl() {
    }

    // CommandBuffer

    CommandBuffer::CommandBuffer(CommandBufferBuilder* builder)
        : CommandBufferBase(builder), mCommands(builder->AcquireCommands()) {
    }

    CommandBuffer::~CommandBuffer() {
        FreeCommands(&mCommands);
    }

    // Queue

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    Queue::~Queue() {
    }

    void Queue::Submit(uint32_t, CommandBuffer* const*) {
        auto operations = ToBackend(GetDevice())->AcquirePendingOperations();

        for (auto& operation : operations) {
            operation->Execute();
        }

        operations.clear();
    }

    // SwapChain

    SwapChain::SwapChain(SwapChainBuilder* builder) : SwapChainBase(builder) {
        const auto& im = GetImplementation();
        im.Init(im.userData, nullptr);
    }

    SwapChain::~SwapChain() {
    }

    TextureBase* SwapChain::GetNextTextureImpl(TextureBuilder* builder) {
        return GetDevice()->CreateTexture(builder);
    }

    void SwapChain::OnBeforePresent(TextureBase*) {
    }

}}  // namespace backend::null
