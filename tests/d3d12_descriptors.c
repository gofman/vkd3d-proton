/*
 * Copyright 2016-2017 Józef Kucia for CodeWeavers
 * Copyright 2020-2021 Philip Rebohle for Valve Corporation
 * Copyright 2020-2021 Joshua Ashton for Valve Corporation
 * Copyright 2020-2021 Hans-Kristian Arntzen for Valve Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define VKD3D_DBG_CHANNEL VKD3D_DBG_CHANNEL_API
#include "d3d12_crosstest.h"

void test_create_descriptor_heap(void)
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    ID3D12Device *device, *tmp_device;
    ID3D12DescriptorHeap *heap;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 16;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);

    refcount = get_refcount(device);
    ok(refcount == 2, "Got unexpected refcount %u.\n", (unsigned int)refcount);
    hr = ID3D12DescriptorHeap_GetDevice(heap, &IID_ID3D12Device, (void **)&tmp_device);
    ok(hr == S_OK, "Failed to get device, hr %#x.\n", hr);
    refcount = get_refcount(device);
    ok(refcount == 3, "Got unexpected refcount %u.\n", (unsigned int)refcount);
    refcount = ID3D12Device_Release(tmp_device);
    ok(refcount == 2, "Got unexpected refcount %u.\n", (unsigned int)refcount);

    check_interface(heap, &IID_ID3D12Object, true);
    check_interface(heap, &IID_ID3D12DeviceChild, true);
    check_interface(heap, &IID_ID3D12Pageable, true);
    check_interface(heap, &IID_ID3D12DescriptorHeap, true);

    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);
    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);
    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);
    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == S_OK, "Failed to create descriptor heap, hr %#x.\n", hr);
    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    refcount = ID3D12Device_Release(device);
    ok(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

void test_descriptor_tables(void)
{
    ID3D12DescriptorHeap *heap, *sampler_heap, *heaps[2];
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_range[4];
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    D3D12_ROOT_PARAMETER root_parameters[3];
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    ID3D12Resource *cb, *textures[4];
    unsigned int i, descriptor_size;
    D3D12_SAMPLER_DESC sampler_desc;
    struct test_context_desc desc;
    D3D12_SUBRESOURCE_DATA data;
    struct test_context context;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t0;
        Texture2D t1;
        Texture2D t2;
        Texture2D t3;
        SamplerState s0;

        cbuffer cb0
        {
            float4 c;
        };

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p = float2(position.x / 32.0f, position.y / 32.0f);

            return c.x * t0.Sample(s0, p) + c.y * t1.Sample(s0, p)
                    + c.z * t2.Sample(s0, p) + c.w * t3.Sample(s0, p);
        }
#endif
        0x43425844, 0xf848ef5f, 0x4da3fe0c, 0x776883a0, 0x6b3f0297, 0x00000001, 0x0000029c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000200, 0x00000050,
        0x00000080, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000,
        0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04001858, 0x00107000, 0x00000001,
        0x00005555, 0x04001858, 0x00107000, 0x00000002, 0x00005555, 0x04001858, 0x00107000, 0x00000003,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000003, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002,
        0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2, 0x00155543, 0x001000f2,
        0x00000001, 0x00100046, 0x00000000, 0x00107e46, 0x00000001, 0x00106000, 0x00000000, 0x08000038,
        0x001000f2, 0x00000001, 0x00100e46, 0x00000001, 0x00208556, 0x00000000, 0x00000000, 0x8b000045,
        0x800000c2, 0x00155543, 0x001000f2, 0x00000002, 0x00100046, 0x00000000, 0x00107e46, 0x00000000,
        0x00106000, 0x00000000, 0x0a000032, 0x001000f2, 0x00000001, 0x00208006, 0x00000000, 0x00000000,
        0x00100e46, 0x00000002, 0x00100e46, 0x00000001, 0x8b000045, 0x800000c2, 0x00155543, 0x001000f2,
        0x00000002, 0x00100046, 0x00000000, 0x00107e46, 0x00000002, 0x00106000, 0x00000000, 0x8b000045,
        0x800000c2, 0x00155543, 0x001000f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000003,
        0x00106000, 0x00000000, 0x0a000032, 0x001000f2, 0x00000001, 0x00208aa6, 0x00000000, 0x00000000,
        0x00100e46, 0x00000002, 0x00100e46, 0x00000001, 0x0a000032, 0x001020f2, 0x00000000, 0x00208ff6,
        0x00000000, 0x00000000, 0x00100e46, 0x00000000, 0x00100e46, 0x00000001, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const struct vec4 constant = {0.1f, 0.2f, 0.3f, 0.1f};
    static const unsigned int texture_data[4] = {0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffff00};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;

    cb = create_upload_buffer(context.device, sizeof(constant), &constant.x);

    descriptor_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[0].NumDescriptors = 2;
    descriptor_range[0].BaseShaderRegister = 0;
    descriptor_range[0].RegisterSpace = 0;
    descriptor_range[0].OffsetInDescriptorsFromTableStart = 1;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_range[0];
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    descriptor_range[1].NumDescriptors = 1;
    descriptor_range[1].BaseShaderRegister = 0;
    descriptor_range[1].RegisterSpace = 0;
    descriptor_range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_range[1];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[2].NumDescriptors = 2;
    descriptor_range[2].BaseShaderRegister = 2;
    descriptor_range[2].RegisterSpace = 0;
    descriptor_range[2].OffsetInDescriptorsFromTableStart = 0;
    descriptor_range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptor_range[3].NumDescriptors = 1;
    descriptor_range[3].BaseShaderRegister = 0;
    descriptor_range[3].RegisterSpace = 0;
    descriptor_range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[2].DescriptorTable.NumDescriptorRanges = 2;
    root_parameters[2].DescriptorTable.pDescriptorRanges = &descriptor_range[2];
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 3;
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(context.device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps, NULL);

    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6);
    sampler_heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(context.device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        textures[i] = create_default_texture(context.device,
                1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_RESOURCE_STATE_COPY_DEST);
        data.pData = &texture_data[i];
        data.RowPitch = sizeof(texture_data[i]);
        data.SlicePitch = data.RowPitch;
        upload_texture_data(textures[i], &data, 1, queue, command_list);
        reset_command_list(command_list, context.allocator);
    }

    for (i = 0; i < ARRAY_SIZE(textures); ++i)
        transition_resource_state(command_list, textures[i],
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    cpu_handle.ptr += descriptor_size;
    /* t0-t3 */
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        ID3D12Device_CreateShaderResourceView(context.device, textures[i], NULL, cpu_handle);
        cpu_handle.ptr += descriptor_size;
    }
    /* cbv0 */
    cbv_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(cb);
    cbv_desc.SizeInBytes = align(sizeof(constant), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ID3D12Device_CreateConstantBufferView(context.device, &cbv_desc, cpu_handle);

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(sampler_heap);
    /* s0 */
    ID3D12Device_CreateSampler(context.device, &sampler_desc, cpu_handle);

    gpu_handle = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    heaps[0] = heap; heaps[1] = sampler_heap;
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, ARRAY_SIZE(heaps), heaps);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0, gpu_handle);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 1,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(sampler_heap));
    gpu_handle.ptr += 3 * descriptor_size;
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 2, gpu_handle);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xb2664c19, 2);

    ID3D12Resource_Release(cb);
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
        ID3D12Resource_Release(textures[i]);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12DescriptorHeap_Release(sampler_heap);
    destroy_test_context(&context);
}

/* Tests overlapping descriptor heap ranges for SRV and UAV descriptor tables.
 * Only descriptors used by the pipeline have to be valid.
 */
void test_descriptor_tables_overlapping_bindings(void)
{
    ID3D12Resource *input_buffers[2], *output_buffers[2];
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_range[2];
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D12_ROOT_PARAMETER root_parameters[3];
    ID3D12GraphicsCommandList *command_list;
    struct resource_readback rb;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    static const DWORD cs_code[] =
    {
#if 0
        ByteAddressBuffer t0;
        ByteAddressBuffer t4 : register(t4);

        RWByteAddressBuffer u0;
        RWByteAddressBuffer u2 : register(u2);

        uint size;
        uint size2;

        [numthreads(1, 1, 1)]
        void main()
        {
            uint i;
            for (i = 0; i < size; ++i)
                u0.Store(4 * i, t0.Load(4 *i));
            for (i = 0; i < size2; ++i)
                u2.Store(4 * i, t4.Load(4 * i));
        }
#endif
        0x43425844, 0x8d2646b7, 0xeb60d9ee, 0x33ccd6ed, 0x5557e649, 0x00000001, 0x0000023c, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000001e8, 0x00050050, 0x0000007a, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x030000a1, 0x00107000, 0x00000000, 0x030000a1,
        0x00107000, 0x00000004, 0x0300009d, 0x0011e000, 0x00000000, 0x0300009d, 0x0011e000, 0x00000002,
        0x02000068, 0x00000001, 0x0400009b, 0x00000001, 0x00000001, 0x00000001, 0x05000036, 0x00100012,
        0x00000000, 0x00004001, 0x00000000, 0x01000030, 0x08000050, 0x00100022, 0x00000000, 0x0010000a,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x03040003, 0x0010001a, 0x00000000, 0x07000029,
        0x00100022, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x890000a5, 0x800002c2,
        0x00199983, 0x00100042, 0x00000000, 0x0010001a, 0x00000000, 0x00107006, 0x00000000, 0x070000a6,
        0x0011e012, 0x00000000, 0x0010001a, 0x00000000, 0x0010002a, 0x00000000, 0x0700001e, 0x00100012,
        0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000001, 0x01000016, 0x05000036, 0x00100012,
        0x00000000, 0x00004001, 0x00000000, 0x01000030, 0x08000050, 0x00100022, 0x00000000, 0x0010000a,
        0x00000000, 0x0020801a, 0x00000000, 0x00000000, 0x03040003, 0x0010001a, 0x00000000, 0x07000029,
        0x00100022, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x890000a5, 0x800002c2,
        0x00199983, 0x00100042, 0x00000000, 0x0010001a, 0x00000000, 0x00107006, 0x00000004, 0x070000a6,
        0x0011e012, 0x00000002, 0x0010001a, 0x00000000, 0x0010002a, 0x00000000, 0x0700001e, 0x00100012,
        0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000001, 0x01000016, 0x0100003e,
    };
    static const uint32_t buffer_data[] = {0xdeadbabe};
    static const uint32_t buffer_data2[] = {0, 1, 2, 3, 4, 5};

    if (!init_compute_test_context(&context))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    descriptor_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[0].NumDescriptors = 10;
    descriptor_range[0].BaseShaderRegister = 0;
    descriptor_range[0].RegisterSpace = 0;
    descriptor_range[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_range[0];
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    descriptor_range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_range[1].NumDescriptors = 10;
    descriptor_range[1].BaseShaderRegister = 0;
    descriptor_range[1].RegisterSpace = 0;
    descriptor_range[1].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_range[1];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[2].Constants.ShaderRegister = 0;
    root_parameters[2].Constants.RegisterSpace = 0;
    root_parameters[2].Constants.Num32BitValues = 2;
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 3;
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_compute_pipeline_state(device, context.root_signature,
            shader_bytecode(cs_code, sizeof(cs_code)));

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 30);

    input_buffers[0] = create_default_buffer(device, sizeof(buffer_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[0], 0, sizeof(buffer_data), &buffer_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    input_buffers[1] = create_default_buffer(device, sizeof(buffer_data2),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[1], 0, sizeof(buffer_data2), &buffer_data2, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[1],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    output_buffers[0] = create_default_buffer(device, sizeof(buffer_data),
              D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    output_buffers[1] = create_default_buffer(device, sizeof(buffer_data2),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = ARRAY_SIZE(buffer_data);
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    ID3D12Device_CreateUnorderedAccessView(device, output_buffers[0], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, heap, 0)); /* u0 */
    uav_desc.Buffer.NumElements = ARRAY_SIZE(buffer_data2);
    ID3D12Device_CreateUnorderedAccessView(device, output_buffers[1], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, heap, 2)); /* u2 */

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = ARRAY_SIZE(buffer_data);
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    ID3D12Device_CreateShaderResourceView(device, input_buffers[0], &srv_desc,
            get_cpu_descriptor_handle(&context, heap, 3)); /* t0 */
    srv_desc.Buffer.NumElements = ARRAY_SIZE(buffer_data2);
    ID3D12Device_CreateShaderResourceView(device, input_buffers[1], &srv_desc,
            get_cpu_descriptor_handle(&context, heap, 7)); /* t4 */

    ID3D12GraphicsCommandList_SetComputeRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 3));
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 1,
            get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 2,
            ARRAY_SIZE(buffer_data), 0);
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 2,
            ARRAY_SIZE(buffer_data2), 1);
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    for (i = 0; i < ARRAY_SIZE(output_buffers); ++i)
    {
        transition_resource_state(command_list, output_buffers[i],
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    }

    get_buffer_readback_with_command_list(output_buffers[0], DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    for (i = 0; i < ARRAY_SIZE(buffer_data); ++i)
    {
        unsigned int value = get_readback_uint(&rb, i, 0, 0);
        ok(value == buffer_data[i], "Got %#x, expected %#x.\n", value, buffer_data[i]);
    }
    release_resource_readback(&rb);
    reset_command_list(command_list, context.allocator);
    get_buffer_readback_with_command_list(output_buffers[1], DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    for (i = 0; i < ARRAY_SIZE(buffer_data2); ++i)
    {
        unsigned int value = get_readback_uint(&rb, i, 0, 0);
        ok(value == buffer_data2[i], "Got %#x, expected %#x.\n", value, buffer_data2[i]);
    }
    release_resource_readback(&rb);

    for (i = 0; i < ARRAY_SIZE(input_buffers); ++i)
        ID3D12Resource_Release(input_buffers[i]);
    for (i = 0; i < ARRAY_SIZE(output_buffers); ++i)
        ID3D12Resource_Release(output_buffers[i]);
    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_update_root_descriptors(void)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_GPU_VIRTUAL_ADDRESS cb_va, uav_va;
    D3D12_ROOT_PARAMETER root_parameters[2];
    ID3D12GraphicsCommandList *command_list;
    ID3D12RootSignature *root_signature;
    ID3D12PipelineState *pipeline_state;
    ID3D12Resource *resource, *cb;
    struct resource_readback rb;
    struct test_context context;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    static const DWORD cs_code[] =
    {
#if 0
        cbuffer cb
        {
            unsigned int offset;
            unsigned int value;
        };

        RWByteAddressBuffer b;

        [numthreads(1, 1, 1)]
        void main()
        {
            b.Store(4 * offset, value);
        }
#endif
        0x43425844, 0xaadc5460, 0x88c27e90, 0x2acacf4e, 0x4e06019a, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x00000084, 0x00050050, 0x00000021, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300009d, 0x0011e000, 0x00000000, 0x02000068,
        0x00000001, 0x0400009b, 0x00000001, 0x00000001, 0x00000001, 0x08000029, 0x00100012, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x00004001, 0x00000002, 0x080000a6, 0x0011e012, 0x00000000,
        0x0010000a, 0x00000000, 0x0020801a, 0x00000000, 0x00000000, 0x0100003e,
    };
    struct
    {
        uint32_t offset;
        uint32_t value;
        uint32_t uav_offset;
        uint8_t padding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 3 * sizeof(uint32_t)];
    }
    input[] =
    {
        {0, 4,  0},
        {2, 6,  0},
        {0, 5, 64},
        {7, 2, 64},
    };

    if (!init_compute_test_context(&context))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    cb = create_upload_buffer(context.device, sizeof(input), input);
    cb_va = ID3D12Resource_GetGPUVirtualAddress(cb);

    resource = create_default_buffer(device, 512,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    uav_va = ID3D12Resource_GetGPUVirtualAddress(resource);

    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[0].Descriptor.ShaderRegister = 0;
    root_parameters[0].Descriptor.RegisterSpace = 0;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    root_parameters[1].Descriptor.ShaderRegister = 0;
    root_parameters[1].Descriptor.RegisterSpace = 0;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 2;
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(device, &root_signature_desc, &root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    pipeline_state = create_compute_pipeline_state(device, root_signature,
            shader_bytecode(cs_code, sizeof(cs_code)));

    ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state);
    ID3D12GraphicsCommandList_SetComputeRootSignature(command_list, root_signature);
    for (i = 0; i < ARRAY_SIZE(input); ++i)
    {
        ID3D12GraphicsCommandList_SetComputeRootConstantBufferView(command_list,
                0, cb_va + i * sizeof(*input));
        if (!i || input[i - 1].uav_offset != input[i].uav_offset)
            ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(command_list,
                    1, uav_va + input[i].uav_offset * sizeof(uint32_t));
        ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);
    }

    transition_sub_resource_state(command_list, resource, 0,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    get_buffer_readback_with_command_list(resource, DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    for (i = 0; i < ARRAY_SIZE(input); ++i)
    {
        unsigned int offset = input[i].uav_offset + input[i].offset;
        unsigned int value = get_readback_uint(&rb, offset, 0, 0);
        ok(value == input[i].value, "Got %#x, expected %#x.\n", value, input[i].value);
    }
    release_resource_readback(&rb);

    ID3D12Resource_Release(cb);
    ID3D12Resource_Release(resource);
    ID3D12RootSignature_Release(root_signature);
    ID3D12PipelineState_Release(pipeline_state);
    destroy_test_context(&context);
}

void test_update_descriptor_tables(void)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_range;
    ID3D12GraphicsCommandList *command_list;
    D3D12_STATIC_SAMPLER_DESC sampler_desc;
    ID3D12DescriptorHeap *heap, *cpu_heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    D3D12_ROOT_PARAMETER root_parameter;
    struct test_context_desc desc;
    D3D12_SUBRESOURCE_DATA data;
    struct resource_readback rb;
    struct test_context context;
    ID3D12Resource *textures[3];
    ID3D12CommandQueue *queue;
    unsigned int i;
    D3D12_BOX box;
    HRESULT hr;
    RECT rect;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t0;
        Texture2D t1;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p = (position.x / 32.0f, position.x / 32.0f);
            return float4(t0.Sample(s, p).r, t1.Sample(s, p).r, 0, 1);
        }
#endif
        0x43425844, 0x5c19caa6, 0xd4fadb4f, 0xc9d6831e, 0x563b68b7, 0x00000001, 0x000001a4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000010f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000108, 0x00000050,
        0x00000042, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04001858, 0x00107000, 0x00000001, 0x00005555, 0x04002064, 0x00101012, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x07000038, 0x00100012,
        0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x3d000000, 0x8b000045, 0x800000c2, 0x00155543,
        0x00100022, 0x00000000, 0x00100006, 0x00000000, 0x00107e16, 0x00000000, 0x00106000, 0x00000000,
        0x8b000045, 0x800000c2, 0x00155543, 0x00100012, 0x00000000, 0x00100006, 0x00000000, 0x00107e46,
        0x00000001, 0x00106000, 0x00000000, 0x05000036, 0x00102032, 0x00000000, 0x00100516, 0x00000000,
        0x08000036, 0x001020c2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
        0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const float texture_data[] = {0.5f, 0.25f, 0.1f};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;

    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.ShaderRegister = 0;
    sampler_desc.RegisterSpace = 0;
    sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range.NumDescriptors = 2;
    descriptor_range.BaseShaderRegister = 0;
    descriptor_range.RegisterSpace = 0;
    descriptor_range.OffsetInDescriptorsFromTableStart = 0;
    root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameter.DescriptorTable.NumDescriptorRanges = 1;
    root_parameter.DescriptorTable.pDescriptorRanges = &descriptor_range;
    root_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 1;
    root_signature_desc.pParameters = &root_parameter;
    root_signature_desc.NumStaticSamplers = 1;
    root_signature_desc.pStaticSamplers = &sampler_desc;
    hr = create_root_signature(context.device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps, NULL);

    memset(&heap_desc, 0, sizeof(heap_desc));
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 4;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = ID3D12Device_CreateDescriptorHeap(context.device, &heap_desc,
            &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(SUCCEEDED(hr), "Failed to create descriptor heap, hr %#x.\n", hr);

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = ID3D12Device_CreateDescriptorHeap(context.device, &heap_desc,
            &IID_ID3D12DescriptorHeap, (void **)&cpu_heap);
    ok(SUCCEEDED(hr), "Failed to create descriptor heap, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        textures[i] = create_default_texture(context.device, 1, 1, DXGI_FORMAT_R32_FLOAT,
                0, D3D12_RESOURCE_STATE_COPY_DEST);
        data.pData = &texture_data[i];
        data.RowPitch = sizeof(texture_data[i]);
        data.SlicePitch = data.RowPitch;
        upload_texture_data(textures[i], &data, 1, queue, command_list);
        reset_command_list(command_list, context.allocator);
        transition_resource_state(command_list, textures[i],
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    for (i = 0; i < heap_desc.NumDescriptors; ++i)
    {
        ID3D12Device_CreateShaderResourceView(context.device, textures[2], NULL,
                get_cpu_descriptor_handle(&context, heap, i));
    }
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        ID3D12Device_CreateShaderResourceView(context.device, textures[i], NULL,
                get_cpu_descriptor_handle(&context, cpu_heap, i));
    }

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);

    set_rect(&rect, 0, 0, 16, 32);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &rect);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12Device_CopyDescriptorsSimple(context.device, 2,
            get_cpu_sampler_handle(&context, heap, 0),
            get_cpu_sampler_handle(&context, cpu_heap, 0),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    set_rect(&rect, 16, 0, 32, 32);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &rect);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 2));
    ID3D12Device_CreateShaderResourceView(context.device, textures[1], NULL,
            get_cpu_descriptor_handle(&context, heap, 2));
    ID3D12Device_CreateShaderResourceView(context.device, textures[0], NULL,
            get_cpu_descriptor_handle(&context, heap, 3));
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_texture_readback_with_command_list(context.render_target, 0, &rb, queue, command_list);
    set_box(&box, 0, 0, 0, 16, 32, 1);
    check_readback_data_uint(&rb, &box, 0xff00407f, 1);
    set_box(&box, 16, 0, 0, 32, 32, 1);
    check_readback_data_uint(&rb, &box, 0xff007f40, 1);
    release_resource_readback(&rb);

    for (i = 0; i < ARRAY_SIZE(textures); ++i)
        ID3D12Resource_Release(textures[i]);
    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_update_descriptor_heap_after_closing_command_list(void)
{
    ID3D12Resource *red_texture, *green_texture;
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    ID3D12DescriptorHeap *cpu_heap, *heap;
    D3D12_SUBRESOURCE_DATA texture_data;
    struct test_context_desc desc;
    struct resource_readback rb;
    struct test_context context;
    ID3D12CommandQueue *queue;
    unsigned int value;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 32.0f;
            p.y = position.y / 32.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x7a0c3929, 0x75ff3ca4, 0xccb318b2, 0xe6965b4c, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000a4, 0x00000050,
        0x00000029, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002,
        0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const unsigned int red_data[] = {0xff0000ff};
    static const unsigned int green_data[] = {0xff00ff00};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;

    context.root_signature = create_texture_root_signature(context.device,
            D3D12_SHADER_VISIBILITY_PIXEL, 0, 0);
    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps, NULL);

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);

    cpu_heap = create_cpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    red_texture = create_default_texture(context.device, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
            0, D3D12_RESOURCE_STATE_COPY_DEST);
    texture_data.pData = red_data;
    texture_data.RowPitch = sizeof(*red_data);
    texture_data.SlicePitch = texture_data.RowPitch;
    upload_texture_data(red_texture, &texture_data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, red_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    green_texture = create_default_texture(context.device, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
            0, D3D12_RESOURCE_STATE_COPY_DEST);
    texture_data.pData = green_data;
    upload_texture_data(green_texture, &texture_data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, green_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ID3D12Device_CreateShaderResourceView(context.device, red_texture, NULL,
            get_cpu_descriptor_handle(&context, cpu_heap, 0));
    ID3D12Device_CopyDescriptorsSimple(context.device, 1,
            get_cpu_sampler_handle(&context, heap, 0),
            get_cpu_sampler_handle(&context, cpu_heap, 0),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);
    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    hr = ID3D12GraphicsCommandList_Close(command_list);
    ok(SUCCEEDED(hr), "Failed to close command list, hr %#x.\n", hr);

    /* Update the descriptor heap used by the closed command list. */
    ID3D12Device_CreateShaderResourceView(context.device, green_texture, NULL, cpu_handle);

    exec_command_list(queue, command_list);
    wait_queue_idle(context.device, queue);
    reset_command_list(command_list, context.allocator);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_texture_readback_with_command_list(context.render_target, 0, &rb, queue, command_list);
    value = get_readback_uint(&rb, 0, 0, 0);
    ok(value == 0xff00ff00, "Got unexpected value %#x.\n", value);
    release_resource_readback(&rb);

    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12Resource_Release(green_texture);
    ID3D12Resource_Release(red_texture);
    destroy_test_context(&context);
}

void test_update_compute_descriptor_tables(void)
{
    struct cb_data
    {
        struct uvec4 srv_size[2];
        struct uvec4 uav_size[2];
    };

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    ID3D12PipelineState *buffer_pso, *texture_pso;
    D3D12_DESCRIPTOR_RANGE descriptor_ranges[4];
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D12GraphicsCommandList *command_list;
    D3D12_ROOT_PARAMETER root_parameters[5];
    D3D12_SUBRESOURCE_DATA subresource_data;
    ID3D12Resource *buffer_cb, *texture_cb;
    ID3D12DescriptorHeap *descriptor_heap;
    ID3D12Resource *output_buffers[2];
    ID3D12Resource *input_buffers[5];
    ID3D12Resource *textures[3];
    struct resource_readback rb;
    struct test_context context;
    ID3D12CommandQueue *queue;
    struct cb_data cb_data;
    ID3D12Device *device;
    unsigned int i;
    uint32_t data;
    HRESULT hr;

    static const DWORD cs_buffer_code[] =
    {
#if 0
        uint offset;

        RWByteAddressBuffer u0 : register(u0);

        cbuffer cb0 : register(b0)
        {
            uint4 srv_size[2];
            uint4 uav_size[2];
        };

        Buffer<uint> t0 : register(t0);
        Buffer<uint> t1 : register(t1);

        RWBuffer<uint> u4 : register(u4);
        RWBuffer<uint> u7 : register(u7);

        [numthreads(1, 1, 1)]
        void main()
        {
            uint x, result, byte_offset = offset;

            for (x = 0, result = 0; x < srv_size[0].x; ++x)
                result += t0.Load(x);
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (x = 0, result = 0; x < srv_size[1].x; ++x)
                result += t1.Load(x);
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (x = 0, result = 0; x < uav_size[0].x; ++x)
                result += u4[x];
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (x = 0, result = 0; x < uav_size[1].x; ++x)
                result += u7[x];
            u0.Store(byte_offset, result);
        }
#endif
        0x43425844, 0xb3d9f052, 0xcc3f0310, 0xd18f8515, 0xccabd8f6, 0x00000001, 0x00000404, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000003b0, 0x00050050, 0x000000ec, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000001, 0x00000001, 0x04000059, 0x00208e46, 0x00000000, 0x00000004,
        0x04000858, 0x00107000, 0x00000000, 0x00004444, 0x04000858, 0x00107000, 0x00000001, 0x00004444,
        0x0300009d, 0x0011e000, 0x00000000, 0x0400089c, 0x0011e000, 0x00000004, 0x00004444, 0x0400089c,
        0x0011e000, 0x00000007, 0x00004444, 0x02000068, 0x00000002, 0x0400009b, 0x00000001, 0x00000001,
        0x00000001, 0x08000036, 0x00100032, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x01000030, 0x08000050, 0x00100012, 0x00000001, 0x0010001a, 0x00000000, 0x0020800a,
        0x00000000, 0x00000000, 0x03040003, 0x0010000a, 0x00000001, 0x8900002d, 0x80000042, 0x00111103,
        0x00100012, 0x00000001, 0x00100556, 0x00000000, 0x00107e46, 0x00000000, 0x0700001e, 0x00100012,
        0x00000000, 0x0010000a, 0x00000000, 0x0010000a, 0x00000001, 0x0700001e, 0x00100022, 0x00000000,
        0x0010001a, 0x00000000, 0x00004001, 0x00000001, 0x01000016, 0x08000036, 0x00100032, 0x00000001,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000030, 0x08000050, 0x00100042,
        0x00000001, 0x0010000a, 0x00000001, 0x0020800a, 0x00000000, 0x00000001, 0x03040003, 0x0010002a,
        0x00000001, 0x8900002d, 0x80000042, 0x00111103, 0x00100042, 0x00000001, 0x00100006, 0x00000001,
        0x00107c96, 0x00000001, 0x0700001e, 0x00100022, 0x00000001, 0x0010001a, 0x00000001, 0x0010002a,
        0x00000001, 0x0700001e, 0x00100012, 0x00000001, 0x0010000a, 0x00000001, 0x00004001, 0x00000001,
        0x01000016, 0x05000036, 0x00100022, 0x00000000, 0x0010001a, 0x00000001, 0x08000036, 0x00100032,
        0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000030, 0x08000050,
        0x00100042, 0x00000001, 0x0010000a, 0x00000001, 0x0020800a, 0x00000000, 0x00000002, 0x03040003,
        0x0010002a, 0x00000001, 0x890000a3, 0x80000042, 0x00111103, 0x00100042, 0x00000001, 0x00100006,
        0x00000001, 0x0011ec96, 0x00000004, 0x0700001e, 0x00100022, 0x00000001, 0x0010001a, 0x00000001,
        0x0010002a, 0x00000001, 0x0700001e, 0x00100012, 0x00000001, 0x0010000a, 0x00000001, 0x00004001,
        0x00000001, 0x01000016, 0x05000036, 0x00100042, 0x00000000, 0x0010001a, 0x00000001, 0x08000036,
        0x00100032, 0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000030,
        0x08000050, 0x00100042, 0x00000001, 0x0010000a, 0x00000001, 0x0020800a, 0x00000000, 0x00000003,
        0x03040003, 0x0010002a, 0x00000001, 0x890000a3, 0x80000042, 0x00111103, 0x00100042, 0x00000001,
        0x00100006, 0x00000001, 0x0011ec96, 0x00000007, 0x0700001e, 0x00100022, 0x00000001, 0x0010001a,
        0x00000001, 0x0010002a, 0x00000001, 0x0700001e, 0x00100012, 0x00000001, 0x0010000a, 0x00000001,
        0x00004001, 0x00000001, 0x01000016, 0x05000036, 0x00100082, 0x00000000, 0x0010001a, 0x00000001,
        0x080000a6, 0x0011e0f2, 0x00000000, 0x0020800a, 0x00000001, 0x00000000, 0x00100e46, 0x00000000,
        0x0100003e,
    };
    static const DWORD cs_texture_code[] =
    {
#if 0
        uint offset;

        RWByteAddressBuffer u0 : register(u0);

        cbuffer cb0 : register(b0)
        {
            uint4 srv_size[2];
            uint4 uav_size[2];
        };

        Texture2D<uint> t0 : register(t0);
        Texture2D<uint> t1 : register(t1);

        RWBuffer<uint> u4 : register(u4);
        RWTexture2D<uint> u6 : register(u6);

        [numthreads(1, 1, 1)]
        void main()
        {
            uint x, y, result, byte_offset = offset;

            for (y = 0, result = 0; y < srv_size[0].y; ++y)
            {
                for (x = 0; x < srv_size[0].x; ++x)
                    result += t0.Load(int3(x, y, 0));
            }
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (y = 0, result = 0; y < srv_size[1].y; ++y)
            {
                for (x = 0; x < srv_size[1].x; ++x)
                    result += t1.Load(int3(x, y, 0));
            }
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (x = 0, result = 0; x < uav_size[0].x; ++x)
                result += u4[x];
            u0.Store(byte_offset, result);
            byte_offset += 4;

            for (y = 0, result = 0; y < uav_size[1].y; ++y)
            {
                for (x = 0; x < uav_size[1].x; ++x)
                    result += u6[uint2(x, y)];
            }
            u0.Store(byte_offset, result);
        }
#endif
        0x43425844, 0x3f0f012e, 0xfb75f6aa, 0xb87ffe68, 0xf25f9ee6, 0x00000001, 0x00000650, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000005fc, 0x00050050, 0x0000017f, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000001, 0x00000001, 0x04000059, 0x00208e46, 0x00000000, 0x00000004,
        0x04001858, 0x00107000, 0x00000000, 0x00004444, 0x04001858, 0x00107000, 0x00000001, 0x00004444,
        0x0300009d, 0x0011e000, 0x00000000, 0x0400089c, 0x0011e000, 0x00000004, 0x00004444, 0x0400189c,
        0x0011e000, 0x00000006, 0x00004444, 0x02000068, 0x00000004, 0x0400009b, 0x00000001, 0x00000001,
        0x00000001, 0x08000036, 0x001000c2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x05000036, 0x00100012, 0x00000001, 0x00004001, 0x00000000, 0x05000036, 0x00100012,
        0x00000002, 0x00004001, 0x00000000, 0x01000030, 0x08000050, 0x00100022, 0x00000001, 0x0010000a,
        0x00000001, 0x0020801a, 0x00000000, 0x00000000, 0x03040003, 0x0010001a, 0x00000001, 0x05000036,
        0x00100022, 0x00000000, 0x0010000a, 0x00000001, 0x05000036, 0x00100012, 0x00000003, 0x00004001,
        0x00000000, 0x05000036, 0x00100022, 0x00000003, 0x0010000a, 0x00000002, 0x01000030, 0x08000050,
        0x00100022, 0x00000001, 0x0010000a, 0x00000003, 0x0020800a, 0x00000000, 0x00000000, 0x03040003,
        0x0010001a, 0x00000001, 0x05000036, 0x00100012, 0x00000000, 0x0010000a, 0x00000003, 0x8900002d,
        0x800000c2, 0x00111103, 0x00100012, 0x00000000, 0x00100e46, 0x00000000, 0x00107e46, 0x00000000,
        0x0700001e, 0x00100022, 0x00000003, 0x0010001a, 0x00000003, 0x0010000a, 0x00000000, 0x0700001e,
        0x00100012, 0x00000003, 0x0010000a, 0x00000003, 0x00004001, 0x00000001, 0x01000016, 0x05000036,
        0x00100012, 0x00000002, 0x0010001a, 0x00000003, 0x0700001e, 0x00100012, 0x00000001, 0x0010000a,
        0x00000001, 0x00004001, 0x00000001, 0x01000016, 0x08000036, 0x001000c2, 0x00000000, 0x00004002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000036, 0x00100012, 0x00000001, 0x00004001,
        0x00000000, 0x05000036, 0x00100022, 0x00000002, 0x00004001, 0x00000000, 0x01000030, 0x08000050,
        0x00100022, 0x00000001, 0x0010000a, 0x00000001, 0x0020801a, 0x00000000, 0x00000001, 0x03040003,
        0x0010001a, 0x00000001, 0x05000036, 0x00100022, 0x00000000, 0x0010000a, 0x00000001, 0x05000036,
        0x00100012, 0x00000003, 0x00004001, 0x00000000, 0x05000036, 0x00100022, 0x00000003, 0x0010001a,
        0x00000002, 0x01000030, 0x08000050, 0x00100022, 0x00000001, 0x0010000a, 0x00000003, 0x0020800a,
        0x00000000, 0x00000001, 0x03040003, 0x0010001a, 0x00000001, 0x05000036, 0x00100012, 0x00000000,
        0x0010000a, 0x00000003, 0x8900002d, 0x800000c2, 0x00111103, 0x00100012, 0x00000000, 0x00100e46,
        0x00000000, 0x00107e46, 0x00000001, 0x0700001e, 0x00100022, 0x00000003, 0x0010001a, 0x00000003,
        0x0010000a, 0x00000000, 0x0700001e, 0x00100012, 0x00000003, 0x0010000a, 0x00000003, 0x00004001,
        0x00000001, 0x01000016, 0x05000036, 0x00100022, 0x00000002, 0x0010001a, 0x00000003, 0x0700001e,
        0x00100012, 0x00000001, 0x0010000a, 0x00000001, 0x00004001, 0x00000001, 0x01000016, 0x08000036,
        0x00100032, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000030,
        0x08000050, 0x00100042, 0x00000000, 0x0010000a, 0x00000000, 0x0020800a, 0x00000000, 0x00000002,
        0x03040003, 0x0010002a, 0x00000000, 0x890000a3, 0x80000042, 0x00111103, 0x00100042, 0x00000000,
        0x00100006, 0x00000000, 0x0011ec96, 0x00000004, 0x0700001e, 0x00100022, 0x00000000, 0x0010001a,
        0x00000000, 0x0010002a, 0x00000000, 0x0700001e, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
        0x00004001, 0x00000001, 0x01000016, 0x05000036, 0x00100042, 0x00000002, 0x0010001a, 0x00000000,
        0x05000036, 0x00100012, 0x00000000, 0x00004001, 0x00000000, 0x05000036, 0x00100082, 0x00000002,
        0x00004001, 0x00000000, 0x01000030, 0x08000050, 0x00100022, 0x00000000, 0x0010000a, 0x00000000,
        0x0020801a, 0x00000000, 0x00000003, 0x03040003, 0x0010001a, 0x00000000, 0x05000036, 0x001000e2,
        0x00000001, 0x00100006, 0x00000000, 0x05000036, 0x00100012, 0x00000003, 0x00004001, 0x00000000,
        0x05000036, 0x00100022, 0x00000003, 0x0010003a, 0x00000002, 0x01000030, 0x08000050, 0x00100022,
        0x00000000, 0x0010000a, 0x00000003, 0x0020800a, 0x00000000, 0x00000003, 0x03040003, 0x0010001a,
        0x00000000, 0x05000036, 0x00100012, 0x00000001, 0x0010000a, 0x00000003, 0x890000a3, 0x800000c2,
        0x00111103, 0x00100022, 0x00000000, 0x00100e46, 0x00000001, 0x0011ee16, 0x00000006, 0x0700001e,
        0x00100022, 0x00000003, 0x0010001a, 0x00000003, 0x0010001a, 0x00000000, 0x0700001e, 0x00100012,
        0x00000003, 0x0010000a, 0x00000003, 0x00004001, 0x00000001, 0x01000016, 0x05000036, 0x00100082,
        0x00000002, 0x0010001a, 0x00000003, 0x0700001e, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
        0x00004001, 0x00000001, 0x01000016, 0x080000a6, 0x0011e0f2, 0x00000000, 0x0020800a, 0x00000001,
        0x00000000, 0x00100e46, 0x00000002, 0x0100003e,
    };
    static const uint32_t buffer0_data[] = {1, 2, 3, 1};
    static const uint32_t buffer1_data[] = {10, 20, 30, 10};
    static const uint32_t buffer2_data[] = {100, 200, 300, 200};
    static const uint32_t buffer3_data[] = {1000, 2000, 2000, 2000};
    static const uint32_t buffer4_data[] = {0, 0, 0, 0};
    static const uint32_t texture0_data[4][4] =
    {
        {1, 0, 0, 0},
        {10000, 100, 1000, 10000},
        {0, 0, 0, 2},
        {0, 30000, 10000, 10},
    };
    static const uint32_t texture1_data[4][4] =
    {
        {6, 0, 0, 0},
        {600, 0, 1000, 60000},
        {0, 40, 0, 0},
        {0, 30000, 0, 0},
    };
    static const uint32_t texture2_data[4][4] =
    {
        {1, 1, 1, 1},
        {2, 2, 2, 2},
        {3, 3, 3, 3},
        {4, 4, 4, 4},
    };
    static const uint32_t expected_output0[] = {7, 70, 800, 7000, 70, 0, 800, 7000, 61113, 91646, 800, 40};
    static const uint32_t expected_output1[] = {61113, 91646, 800, 40, 7, 70, 800, 7000};

    if (!init_compute_test_context(&context))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[0].Constants.ShaderRegister = 1;
    root_parameters[0].Constants.RegisterSpace = 0;
    root_parameters[0].Constants.Num32BitValues = 1;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[0].NumDescriptors = 1;
    descriptor_ranges[0].BaseShaderRegister = 0;
    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_ranges[0];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[1].NumDescriptors = 2;
    descriptor_ranges[1].BaseShaderRegister = 0;
    descriptor_ranges[1].RegisterSpace = 0;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[2].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[2].DescriptorTable.pDescriptorRanges = &descriptor_ranges[1];
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[2].NumDescriptors = 4;
    descriptor_ranges[2].BaseShaderRegister = 4;
    descriptor_ranges[2].RegisterSpace = 0;
    descriptor_ranges[2].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[3].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[3].DescriptorTable.pDescriptorRanges = &descriptor_ranges[2];
    root_parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptor_ranges[3].NumDescriptors = 1;
    descriptor_ranges[3].BaseShaderRegister = 0;
    descriptor_ranges[3].RegisterSpace = 0;
    descriptor_ranges[3].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[4].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[4].DescriptorTable.pDescriptorRanges = &descriptor_ranges[3];
    root_parameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_signature_desc.NumParameters = 5;
    root_signature_desc.pParameters = root_parameters;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = NULL;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    buffer_pso = create_compute_pipeline_state(device, context.root_signature,
            shader_bytecode(cs_buffer_code, sizeof(cs_buffer_code)));
    texture_pso = create_compute_pipeline_state(device, context.root_signature,
            shader_bytecode(cs_texture_code, sizeof(cs_texture_code)));

    for (i = 0; i < ARRAY_SIZE(output_buffers); ++i)
    {
        output_buffers[i] = create_default_buffer(device, 1024,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    input_buffers[0] = create_default_buffer(device, sizeof(buffer0_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[0], 0, sizeof(buffer0_data), buffer0_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    input_buffers[1] = create_default_buffer(device, sizeof(buffer1_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[1], 0, sizeof(buffer1_data), buffer1_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[1],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    input_buffers[2] = create_default_buffer(device, sizeof(buffer2_data),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[2], 0, sizeof(buffer2_data), buffer2_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[2],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    input_buffers[3] = create_default_buffer(device, sizeof(buffer3_data),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[3], 0, sizeof(buffer3_data), buffer3_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[3],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    input_buffers[4] = create_default_buffer(device, sizeof(buffer4_data),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(input_buffers[4], 0, sizeof(buffer4_data), buffer4_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, input_buffers[4],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    textures[0] = create_default_texture(context.device,
            4, 4, DXGI_FORMAT_R32_UINT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    subresource_data.pData = texture0_data;
    subresource_data.RowPitch = sizeof(*texture0_data);
    subresource_data.SlicePitch = subresource_data.RowPitch;
    upload_texture_data(textures[0], &subresource_data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, textures[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    textures[1] = create_default_texture(context.device,
            4, 4, DXGI_FORMAT_R32_UINT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    subresource_data.pData = texture1_data;
    subresource_data.RowPitch = sizeof(*texture1_data);
    subresource_data.SlicePitch = subresource_data.RowPitch;
    upload_texture_data(textures[1], &subresource_data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, textures[1],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    textures[2] = create_default_texture(context.device, 4, 4, DXGI_FORMAT_R32_UINT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    subresource_data.pData = texture2_data;
    subresource_data.RowPitch = sizeof(*texture2_data);
    subresource_data.SlicePitch = subresource_data.RowPitch;
    upload_texture_data(textures[2], &subresource_data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, textures[2],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&cb_data, 0, sizeof(cb_data));
    cb_data.srv_size[0].x = ARRAY_SIZE(buffer0_data);
    cb_data.srv_size[1].x = ARRAY_SIZE(buffer1_data);
    cb_data.uav_size[0].x = ARRAY_SIZE(buffer2_data);
    cb_data.uav_size[1].x = ARRAY_SIZE(buffer3_data);
    buffer_cb = create_upload_buffer(device, sizeof(cb_data), &cb_data);

    memset(&cb_data, 0, sizeof(cb_data));
    cb_data.srv_size[0].x = 4;
    cb_data.srv_size[0].y = 4;
    cb_data.srv_size[1].x = 4;
    cb_data.srv_size[1].y = 4;
    cb_data.uav_size[0].x = ARRAY_SIZE(buffer2_data);
    cb_data.uav_size[1].x = 4;
    cb_data.uav_size[1].y = 4;
    texture_cb = create_upload_buffer(device, sizeof(cb_data), &cb_data);

    descriptor_heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 30);

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_UINT;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = ARRAY_SIZE(buffer0_data);
    ID3D12Device_CreateShaderResourceView(device, input_buffers[0], &srv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 0));
    srv_desc.Buffer.NumElements = ARRAY_SIZE(buffer1_data);
    ID3D12Device_CreateShaderResourceView(device, input_buffers[1], &srv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 1));

    ID3D12Device_CreateShaderResourceView(device, input_buffers[1], &srv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 6));
    srv_desc.Buffer.NumElements = ARRAY_SIZE(buffer4_data);
    ID3D12Device_CreateShaderResourceView(device, input_buffers[4], &srv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 7));

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_UINT;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = ARRAY_SIZE(buffer2_data);
    ID3D12Device_CreateUnorderedAccessView(device, input_buffers[2], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 2));
    ID3D12Device_CreateUnorderedAccessView(device, input_buffers[2], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 12));
    uav_desc.Buffer.NumElements = ARRAY_SIZE(buffer3_data);
    ID3D12Device_CreateUnorderedAccessView(device, input_buffers[3], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 5));

    ID3D12Device_CreateShaderResourceView(device, textures[0], NULL,
            get_cpu_descriptor_handle(&context, descriptor_heap, 10));
    ID3D12Device_CreateShaderResourceView(device, textures[1], NULL,
            get_cpu_descriptor_handle(&context, descriptor_heap, 11));

    ID3D12Device_CreateUnorderedAccessView(device, textures[2], NULL, NULL,
            get_cpu_descriptor_handle(&context, descriptor_heap, 14));

    cbv_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(buffer_cb);
    cbv_desc.SizeInBytes = align(sizeof(cb_data), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ID3D12Device_CreateConstantBufferView(context.device, &cbv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 8));

    cbv_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(texture_cb);
    cbv_desc.SizeInBytes = align(sizeof(cb_data), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ID3D12Device_CreateConstantBufferView(context.device, &cbv_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 9));

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = 256;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    ID3D12Device_CreateUnorderedAccessView(device, output_buffers[0], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 20));
    ID3D12Device_CreateUnorderedAccessView(device, output_buffers[1], NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, descriptor_heap, 21));

    ID3D12GraphicsCommandList_SetComputeRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &descriptor_heap);

    ID3D12GraphicsCommandList_SetPipelineState(command_list, buffer_pso);

    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 0, 0 /* offset */, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            1, get_gpu_descriptor_handle(&context, descriptor_heap, 20)); /* u0 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            2, get_gpu_descriptor_handle(&context, descriptor_heap, 0)); /* t1-t2 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            3, get_gpu_descriptor_handle(&context, descriptor_heap, 2)); /* u4-u7 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            4, get_gpu_descriptor_handle(&context, descriptor_heap, 8)); /* b0 */
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 0, 16 /* offset */, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            2, get_gpu_descriptor_handle(&context, descriptor_heap, 6));  /* t1-t2 */
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    ID3D12GraphicsCommandList_SetPipelineState(command_list, texture_pso);

    transition_resource_state(command_list, input_buffers[4],
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 0, 32 /* offset */, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            2, get_gpu_descriptor_handle(&context, descriptor_heap, 10)); /* t1-t2 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            3, get_gpu_descriptor_handle(&context, descriptor_heap, 12)); /* u4-u7 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            4, get_gpu_descriptor_handle(&context, descriptor_heap, 9)); /* b0 */
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 0, 0 /* offset */, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            1, get_gpu_descriptor_handle(&context, descriptor_heap, 21)); /* u0 */
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    ID3D12GraphicsCommandList_SetPipelineState(command_list, buffer_pso);

    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 0, 16 /* offset */, 0);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            2, get_gpu_descriptor_handle(&context, descriptor_heap, 0)); /* t1-t2 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            3, get_gpu_descriptor_handle(&context, descriptor_heap, 2)); /* u4-u7 */
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list,
            4, get_gpu_descriptor_handle(&context, descriptor_heap, 8)); /* b0 */
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    transition_sub_resource_state(command_list, output_buffers[0], 0,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(output_buffers[0], DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    for (i = 0; i < ARRAY_SIZE(expected_output0); ++i)
    {
        data = get_readback_uint(&rb, i, 0, 0);
        ok(data == expected_output0[i], "Got %#x, expected %#x at %u.\n", data, expected_output0[i], i);
    }
    release_resource_readback(&rb);

    reset_command_list(command_list, context.allocator);
    transition_sub_resource_state(command_list, output_buffers[1], 0,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(output_buffers[1], DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    for (i = 0; i < ARRAY_SIZE(expected_output1); ++i)
    {
        data = get_readback_uint(&rb, i, 0, 0);
        ok(data == expected_output1[i], "Got %#x, expected %#x at %u.\n", data, expected_output1[i], i);
    }
    release_resource_readback(&rb);

    ID3D12Resource_Release(buffer_cb);
    ID3D12Resource_Release(texture_cb);
    for (i = 0; i < ARRAY_SIZE(input_buffers); ++i)
        ID3D12Resource_Release(input_buffers[i]);
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
        ID3D12Resource_Release(textures[i]);
    for (i = 0; i < ARRAY_SIZE(output_buffers); ++i)
        ID3D12Resource_Release(output_buffers[i]);
    ID3D12PipelineState_Release(buffer_pso);
    ID3D12PipelineState_Release(texture_pso);
    ID3D12DescriptorHeap_Release(descriptor_heap);
    destroy_test_context(&context);
}

void test_update_descriptor_tables_after_root_signature_change(void)
{
    ID3D12RootSignature *root_signature, *root_signature2;
    ID3D12PipelineState *pipeline_state, *pipeline_state2;
    ID3D12DescriptorHeap *heap, *sampler_heap, *heaps[2];
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_range[4];
    D3D12_ROOT_PARAMETER root_parameters[3];
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    unsigned int i, descriptor_size;
    D3D12_SAMPLER_DESC sampler_desc;
    struct test_context_desc desc;
    ID3D12Resource *textures[2];
    D3D12_SUBRESOURCE_DATA data;
    struct test_context context;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 32.0f;
            p.y = position.y / 32.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x7a0c3929, 0x75ff3ca4, 0xccb318b2, 0xe6965b4c, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000a4, 0x00000050,
        0x00000029, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002,
        0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const unsigned int texture_data[] = {0xff00ff00, 0xff0000ff};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;

    descriptor_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[0].NumDescriptors = 2;
    descriptor_range[0].BaseShaderRegister = 0;
    descriptor_range[0].RegisterSpace = 0;
    descriptor_range[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_range[0];
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    descriptor_range[1].NumDescriptors = 1;
    descriptor_range[1].BaseShaderRegister = 0;
    descriptor_range[1].RegisterSpace = 0;
    descriptor_range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_range[1];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[2].NumDescriptors = 2;
    descriptor_range[2].BaseShaderRegister = 2;
    descriptor_range[2].RegisterSpace = 0;
    descriptor_range[2].OffsetInDescriptorsFromTableStart = 0;
    descriptor_range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptor_range[3].NumDescriptors = 1;
    descriptor_range[3].BaseShaderRegister = 0;
    descriptor_range[3].RegisterSpace = 0;
    descriptor_range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[2].DescriptorTable.NumDescriptorRanges = 2;
    root_parameters[2].DescriptorTable.pDescriptorRanges = &descriptor_range[2];
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters);
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(context.device, &root_signature_desc, &root_signature);
    ok(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);
    root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters) - 1;
    hr = create_root_signature(context.device, &root_signature_desc, &root_signature2);
    ok(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);

    pipeline_state = create_pipeline_state(context.device,
            root_signature, context.render_target_desc.Format, NULL, &ps, NULL);
    pipeline_state2 = create_pipeline_state(context.device,
            root_signature2, context.render_target_desc.Format, NULL, &ps, NULL);

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6);
    sampler_heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    ID3D12Device_CreateSampler(context.device, &sampler_desc, get_cpu_descriptor_handle(&context, sampler_heap, 0));

    descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(context.device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        textures[i] = create_default_texture(context.device,
                1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_RESOURCE_STATE_COPY_DEST);
        data.pData = &texture_data[i];
        data.RowPitch = sizeof(texture_data[i]);
        data.SlicePitch = data.RowPitch;
        upload_texture_data(textures[i], &data, 1, queue, command_list);
        reset_command_list(command_list, context.allocator);
    }

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
    {
        transition_resource_state(command_list, textures[i],
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ID3D12Device_CreateShaderResourceView(context.device, textures[i], NULL, cpu_handle);
        cpu_handle.ptr += descriptor_size;
    }
    for (; i < 6; ++i)
    {
        ID3D12Device_CreateShaderResourceView(context.device, textures[1], NULL, cpu_handle);
        cpu_handle.ptr += descriptor_size;
    }

    heaps[0] = heap; heaps[1] = sampler_heap;
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, ARRAY_SIZE(heaps), heaps);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state);

    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);

    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 1,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(sampler_heap));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 2,
            get_gpu_descriptor_handle(&context, heap, 2));

    ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state2);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, root_signature2);

    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 1,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(sampler_heap));

    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xff00ff00, 0);

    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, ARRAY_SIZE(heaps), heaps);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state2);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, root_signature2);

    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);

    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 1,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(sampler_heap));

    ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, root_signature);

    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xff00ff00, 0);

    ID3D12PipelineState_Release(pipeline_state);
    ID3D12PipelineState_Release(pipeline_state2);
    ID3D12RootSignature_Release(root_signature);
    ID3D12RootSignature_Release(root_signature2);
    for (i = 0; i < ARRAY_SIZE(textures); ++i)
        ID3D12Resource_Release(textures[i]);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12DescriptorHeap_Release(sampler_heap);
    destroy_test_context(&context);
}

void test_copy_descriptors(void)
{
    struct data
    {
        unsigned int u[3];
        float f;
    };

    ID3D12DescriptorHeap *cpu_heap, *cpu_sampler_heap, *cpu_sampler_heap2;
    D3D12_CPU_DESCRIPTOR_HANDLE dst_handles[4], src_handles[4];
    ID3D12DescriptorHeap *heap, *sampler_heap, *heaps[2];
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_ranges[5];
    UINT dst_range_sizes[4], src_range_sizes[4];
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D12GraphicsCommandList *command_list;
    D3D12_ROOT_PARAMETER root_parameters[4];
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    ID3D12Resource *t[7], *u[3], *cb;
    struct depth_stencil_resource ds;
    D3D12_SAMPLER_DESC sampler_desc;
    struct test_context_desc desc;
    unsigned int descriptor_size;
    D3D12_SUBRESOURCE_DATA data;
    struct resource_readback rb;
    struct test_context context;
    ID3D12CommandQueue *queue;
    unsigned int sampler_size;
    ID3D12Device *device;
    unsigned int *result;
    unsigned int i;
    HRESULT hr;

    static const DWORD cs_code[] =
    {
#if 0
        struct data
        {
            uint3 u;
            float f;
        };

        cbuffer cb0
        {
            float f;
        };

        cbuffer cb1
        {
            uint u;
        };

        cbuffer cb2
        {
            int i;
        };

        SamplerState s0;
        SamplerState s1;
        SamplerState s2;
        SamplerComparisonState s3;

        Texture2D t0;
        Texture2D<uint> t1;
        Texture2D<int> t2;
        Buffer<float> t3;
        StructuredBuffer<float> t4;
        ByteAddressBuffer t5;
        Texture2D t6;

        RWByteAddressBuffer u0;
        RWStructuredBuffer<data> u1;

        RWByteAddressBuffer u2;

        [numthreads(1, 1, 1)]
        void main()
        {
            u2.Store(0 * 4, f);
            u2.Store(1 * 4, u);
            u2.Store(2 * 4, i);
            u2.Store(3 * 4, 0);

            u2.Store4( 4 * 4, t0.SampleLevel(s0, (float2)0, 0));
            u2.Store4( 8 * 4, t0.SampleLevel(s1, (float2)0, 0));
            u2.Store4(12 * 4, t0.SampleLevel(s2, (float2)0, 0));

            u2.Store(16 * 4, t1.Load((int3)0));
            u2.Store(17 * 4, t2.Load((int3)0));
            u2.Store(18 * 4, t3.Load(0));
            u2.Store(19 * 4, t4[0]);

            u2.Store4(20 * 4, t5.Load4(0));

            u2.Store4(24 * 4, t6.SampleCmpLevelZero(s3, (float2)0, 0.6f));
            u2.Store4(28 * 4, t6.SampleCmpLevelZero(s3, (float2)0, 0.4f));

            u2.Store2(32 * 4, u0.Load2(0));
            u2.Store2(34 * 4, u0.Load2(8));

            u2.Store3(36 * 4, u1[0].u);
            u2.Store4(39 * 4, u1[0].f);

            u2.Store(43 * 4, 0xdeadbeef);
        }
#endif
        0x43425844, 0x52d2c2d3, 0xaf60e190, 0xb897944f, 0x4a6a6653, 0x00000001, 0x00000650, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000005fc, 0x00050050, 0x0000017f, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04000059, 0x00208e46, 0x00000001, 0x00000001,
        0x04000059, 0x00208e46, 0x00000002, 0x00000001, 0x0300005a, 0x00106000, 0x00000000, 0x0300005a,
        0x00106000, 0x00000001, 0x0300005a, 0x00106000, 0x00000002, 0x0300085a, 0x00106000, 0x00000003,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04001858, 0x00107000, 0x00000001, 0x00004444,
        0x04001858, 0x00107000, 0x00000002, 0x00003333, 0x04000858, 0x00107000, 0x00000003, 0x00005555,
        0x040000a2, 0x00107000, 0x00000004, 0x00000004, 0x030000a1, 0x00107000, 0x00000005, 0x04001858,
        0x00107000, 0x00000006, 0x00005555, 0x0300009d, 0x0011e000, 0x00000000, 0x0400009e, 0x0011e000,
        0x00000001, 0x00000010, 0x0300009d, 0x0011e000, 0x00000002, 0x02000068, 0x00000002, 0x0400009b,
        0x00000001, 0x00000001, 0x00000001, 0x0600001c, 0x00100012, 0x00000000, 0x0020800a, 0x00000000,
        0x00000000, 0x06000036, 0x00100022, 0x00000000, 0x0020800a, 0x00000001, 0x00000000, 0x06000036,
        0x00100042, 0x00000000, 0x0020800a, 0x00000002, 0x00000000, 0x05000036, 0x00100082, 0x00000000,
        0x00004001, 0x00000000, 0x070000a6, 0x0011e0f2, 0x00000002, 0x00004001, 0x00000000, 0x00100e46,
        0x00000000, 0x90000048, 0x800000c2, 0x00155543, 0x001000f2, 0x00000000, 0x00004002, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x00004001,
        0x00000000, 0x0500001c, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x070000a6, 0x0011e0f2,
        0x00000002, 0x00004001, 0x00000010, 0x00100e46, 0x00000000, 0x90000048, 0x800000c2, 0x00155543,
        0x001000f2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00107e46,
        0x00000000, 0x00106000, 0x00000001, 0x00004001, 0x00000000, 0x0500001c, 0x001000f2, 0x00000000,
        0x00100e46, 0x00000000, 0x070000a6, 0x0011e0f2, 0x00000002, 0x00004001, 0x00000020, 0x00100e46,
        0x00000000, 0x90000048, 0x800000c2, 0x00155543, 0x001000f2, 0x00000000, 0x00004002, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000002, 0x00004001,
        0x00000000, 0x0500001c, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x070000a6, 0x0011e0f2,
        0x00000002, 0x00004001, 0x00000030, 0x00100e46, 0x00000000, 0x8c00002d, 0x80000042, 0x00155543,
        0x00100012, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00107e46,
        0x00000003, 0x0500001c, 0x00100042, 0x00000000, 0x0010000a, 0x00000000, 0x8b0000a7, 0x80002302,
        0x00199983, 0x00100012, 0x00000001, 0x00004001, 0x00000000, 0x00004001, 0x00000000, 0x00107006,
        0x00000004, 0x0500001c, 0x00100082, 0x00000000, 0x0010000a, 0x00000001, 0x8c00002d, 0x800000c2,
        0x00111103, 0x00100012, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00107e46, 0x00000001, 0x8c00002d, 0x800000c2, 0x000cccc3, 0x00100022, 0x00000000, 0x00004002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00107e16, 0x00000002, 0x070000a6, 0x0011e0f2,
        0x00000002, 0x00004001, 0x00000040, 0x00100e46, 0x00000000, 0x890000a5, 0x800002c2, 0x00199983,
        0x001000f2, 0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000005, 0x070000a6, 0x0011e0f2,
        0x00000002, 0x00004001, 0x00000050, 0x00100e46, 0x00000000, 0x90000047, 0x800000c2, 0x00155543,
        0x00100012, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00107006,
        0x00000006, 0x00106000, 0x00000003, 0x00004001, 0x3f19999a, 0x0500001c, 0x00100012, 0x00000000,
        0x0010000a, 0x00000000, 0x070000a6, 0x0011e0f2, 0x00000002, 0x00004001, 0x00000060, 0x00100006,
        0x00000000, 0x90000047, 0x800000c2, 0x00155543, 0x00100012, 0x00000000, 0x00004002, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00107006, 0x00000006, 0x00106000, 0x00000003, 0x00004001,
        0x3ecccccd, 0x0500001c, 0x00100012, 0x00000000, 0x0010000a, 0x00000000, 0x070000a6, 0x0011e0f2,
        0x00000002, 0x00004001, 0x00000070, 0x00100006, 0x00000000, 0x890000a5, 0x800002c2, 0x00199983,
        0x00100032, 0x00000000, 0x00004001, 0x00000000, 0x0011e046, 0x00000000, 0x890000a5, 0x800002c2,
        0x00199983, 0x001000c2, 0x00000000, 0x00004001, 0x00000008, 0x0011e406, 0x00000000, 0x070000a6,
        0x0011e0f2, 0x00000002, 0x00004001, 0x00000080, 0x00100e46, 0x00000000, 0x8b0000a7, 0x80008302,
        0x00199983, 0x001000f2, 0x00000000, 0x00004001, 0x00000000, 0x00004001, 0x00000000, 0x0011ee46,
        0x00000001, 0x070000a6, 0x0011e072, 0x00000002, 0x00004001, 0x00000090, 0x00100246, 0x00000000,
        0x0500001c, 0x00100012, 0x00000000, 0x0010003a, 0x00000000, 0x070000a6, 0x0011e0f2, 0x00000002,
        0x00004001, 0x0000009c, 0x00100006, 0x00000000, 0x070000a6, 0x0011e012, 0x00000002, 0x00004001,
        0x000000ac, 0x00004001, 0xdeadbeef, 0x0100003e,
    };
    static const float cb0_data = 10.0f;
    static const UINT cb1_data = 11;
    static const INT cb2_data = -1;
    static const struct vec4 t0_data = {1.0f, 2.0f, 3.0f, 4.0f};
    static const UINT t1_data = 111;
    static const INT t2_data = 222;
    static const float t3_data = 333.3f;
    static const float t4_data = 44.44f;
    static const struct uvec4 t5_data = {50, 51, 52, 53};
    static const struct uvec4 u0_data = {10, 20, 30, 40};
    static const struct data u1_data = {{5, 6, 7}, 10.0f};

    memset(&desc, 0, sizeof(desc));
    desc.no_render_target = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    sampler_size = ID3D12Device_GetDescriptorHandleIncrementSize(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    cpu_sampler_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2);
    cpu_sampler_heap2 = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2);
    sampler_heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 4);

    cpu_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 30);
    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 30);

    /* create samplers */
    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(cpu_sampler_heap);
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    ID3D12Device_CreateSampler(context.device, &sampler_desc, cpu_handle);
    sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
    cpu_handle.ptr += sampler_size;
    ID3D12Device_CreateSampler(context.device, &sampler_desc, cpu_handle);

    /* create CBVs */
    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(cpu_heap);
    cb = create_upload_buffer(context.device,
            3 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, NULL);
    update_buffer_data(cb, 0, sizeof(cb0_data), &cb0_data);
    update_buffer_data(cb, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(cb1_data), &cb1_data);
    update_buffer_data(cb, 2 * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(cb2_data), &cb2_data);
    cbv_desc.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(cb);
    cbv_desc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    for (i = 0; i < 3; ++i)
    {
        ID3D12Device_CreateConstantBufferView(context.device, &cbv_desc, cpu_handle);
        cbv_desc.BufferLocation += D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        cpu_handle.ptr += descriptor_size;
    }

    /* create SRVs */
    cpu_handle = get_cpu_descriptor_handle(&context, cpu_heap, 10);

    t[0] = create_default_texture(context.device,
            1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = &t0_data;
    data.RowPitch = sizeof(t0_data);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(t[0], &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    t[1] = create_default_texture(context.device,
            1, 1, DXGI_FORMAT_R32_UINT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = &t1_data;
    data.RowPitch = sizeof(t1_data);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(t[1], &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[1],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    t[2] = create_default_texture(context.device,
            1, 1, DXGI_FORMAT_R32_SINT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = &t2_data;
    data.RowPitch = sizeof(t2_data);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(t[2], &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[2],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    t[3] = create_default_buffer(device, sizeof(t3_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(t[3], 0, sizeof(t3_data), &t3_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[3],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    t[4] = create_default_buffer(device, sizeof(t4_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(t[4], 0, sizeof(t4_data), &t4_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[4],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    t[5] = create_default_buffer(device, sizeof(t5_data),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(t[5], 0, sizeof(t5_data), &t5_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, t[5],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    init_depth_stencil(&ds, device, 32, 32, 1, 1, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, NULL);
    t[6] = ds.texture;
    ID3D12Resource_AddRef(t[6]);
    ID3D12GraphicsCommandList_ClearDepthStencilView(command_list, ds.dsv_handle,
            D3D12_CLEAR_FLAG_DEPTH, 0.5f, 0, 0, NULL);
    transition_resource_state(command_list, t[6],
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    for (i = 0; i < 3; ++i)
    {
        ID3D12Device_CreateShaderResourceView(device, t[i], NULL, cpu_handle);
        cpu_handle.ptr += descriptor_size;
    }

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = 1;
    ID3D12Device_CreateShaderResourceView(device, t[3], &srv_desc, cpu_handle);
    cpu_handle.ptr += descriptor_size;

    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Buffer.StructureByteStride = sizeof(t4_data);
    ID3D12Device_CreateShaderResourceView(device, t[4], &srv_desc, cpu_handle);
    cpu_handle.ptr += descriptor_size;

    srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.Buffer.NumElements = 4;
    srv_desc.Buffer.StructureByteStride = 0;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    ID3D12Device_CreateShaderResourceView(device, t[5], &srv_desc, cpu_handle);
    cpu_handle.ptr += descriptor_size;

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MipLevels = 1;
    ID3D12Device_CreateShaderResourceView(device, t[6], &srv_desc, cpu_handle);

    /* create UAVs */
    cpu_handle = get_cpu_descriptor_handle(&context, cpu_heap, 20);

    u[0] = create_default_buffer(device, sizeof(u0_data),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(u[0], 0, sizeof(u0_data), &u0_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, u[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    u[1] = create_default_buffer(device, sizeof(struct uvec4),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_buffer_data(u[1], 0, sizeof(u1_data), &u1_data, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, u[0],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    u[2] = create_default_buffer(device, 44 * 4,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = 4;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    ID3D12Device_CreateUnorderedAccessView(device, u[0], NULL, &uav_desc, cpu_handle);
    cpu_handle.ptr += descriptor_size;

    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.Buffer.NumElements = 1;
    uav_desc.Buffer.StructureByteStride = sizeof(u1_data);
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    ID3D12Device_CreateUnorderedAccessView(device, u[1], NULL, &uav_desc, cpu_handle);
    cpu_handle.ptr += descriptor_size;

    uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    uav_desc.Buffer.NumElements = 44;
    uav_desc.Buffer.StructureByteStride = 0;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    ID3D12Device_CreateUnorderedAccessView(device, u[2], NULL, &uav_desc, cpu_handle);

    /* root signature */
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptor_ranges[0].NumDescriptors = 3;
    descriptor_ranges[0].BaseShaderRegister = 0;
    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_ranges[0];
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    descriptor_ranges[1].NumDescriptors = 4;
    descriptor_ranges[1].BaseShaderRegister = 0;
    descriptor_ranges[1].RegisterSpace = 0;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_ranges[1];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[2].NumDescriptors = 7;
    descriptor_ranges[2].BaseShaderRegister = 0;
    descriptor_ranges[2].RegisterSpace = 0;
    descriptor_ranges[2].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[2].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[2].DescriptorTable.pDescriptorRanges = &descriptor_ranges[2];
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptor_ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[3].NumDescriptors = 2;
    descriptor_ranges[3].BaseShaderRegister = 0;
    descriptor_ranges[3].RegisterSpace = 0;
    descriptor_ranges[3].OffsetInDescriptorsFromTableStart = 0;
    descriptor_ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[4].NumDescriptors = 1;
    descriptor_ranges[4].BaseShaderRegister = 2;
    descriptor_ranges[4].RegisterSpace = 0;
    descriptor_ranges[4].OffsetInDescriptorsFromTableStart = 2;
    root_parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[3].DescriptorTable.NumDescriptorRanges = 2;
    root_parameters[3].DescriptorTable.pDescriptorRanges = &descriptor_ranges[3];
    root_parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 4;
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_compute_pipeline_state(device, context.root_signature,
            shader_bytecode(cs_code, sizeof(cs_code)));

    /* copy descriptors */
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 5);
    dst_range_sizes[0] = 2;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    src_range_sizes[0] = 2;
    /* cb0-cb1 */
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, dst_range_sizes,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 7);
    dst_range_sizes[0] = 1;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 2);
    src_range_sizes[0] = 1;
    /* cb2 */
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, dst_range_sizes,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12Device_CopyDescriptorsSimple(device, 2,
            get_cpu_sampler_handle(&context, cpu_sampler_heap2, 0),
            get_cpu_sampler_handle(&context, cpu_sampler_heap, 0),
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    dst_handles[0] = get_cpu_sampler_handle(&context, sampler_heap, 0);
    dst_range_sizes[0] = 4;
    src_handles[0] = get_cpu_sampler_handle(&context, cpu_sampler_heap2, 0);
    src_handles[1] = get_cpu_sampler_handle(&context, cpu_sampler_heap2, 0);
    src_handles[2] = get_cpu_sampler_handle(&context, cpu_sampler_heap2, 0);
    src_handles[3] = get_cpu_sampler_handle(&context, cpu_sampler_heap2, 1);
    /* s0-s3 */
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, dst_range_sizes,
            4, src_handles, NULL, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 9);
    dst_range_sizes[0] = 4;
    dst_handles[1] = get_cpu_descriptor_handle(&context, heap, 9);
    dst_range_sizes[1] = 0;
    dst_handles[2] = get_cpu_descriptor_handle(&context, heap, 13);
    dst_range_sizes[2] = 3;
    dst_handles[3] = get_cpu_descriptor_handle(&context, heap, 13);
    dst_range_sizes[3] = 0;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 10);
    src_range_sizes[0] = 8;
    /* t0-t6 */
    ID3D12Device_CopyDescriptors(device, 4, dst_handles, dst_range_sizes,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    /* copy 1 uninitialized descriptor (19) */
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 19);
    dst_range_sizes[0] = 2;
    dst_handles[1] = get_cpu_descriptor_handle(&context, heap, 21);
    dst_range_sizes[1] = 1;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 19);
    src_range_sizes[0] = 2;
    src_handles[1] = get_cpu_descriptor_handle(&context, cpu_heap, 21);
    src_range_sizes[1] = 1;
    /* u1-u2 */
    ID3D12Device_CopyDescriptors(device, 2, dst_handles, dst_range_sizes,
            2, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    /* u2 */
    ID3D12Device_CopyDescriptorsSimple(device, 1,
            get_cpu_descriptor_handle(&context, heap, 22),
            get_cpu_descriptor_handle(&context, cpu_heap, 22),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    /* range sizes equal to 0 */
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 19);
    dst_range_sizes[0] = 0;
    dst_handles[1] = get_cpu_descriptor_handle(&context, heap, 19);
    dst_range_sizes[1] = 0;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    src_range_sizes[0] = 1;
    src_handles[1] = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    src_range_sizes[1] = 4;
    ID3D12Device_CopyDescriptors(device, 2, dst_handles, dst_range_sizes,
            2, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 19);
    dst_range_sizes[0] = 4;
    dst_handles[1] = get_cpu_descriptor_handle(&context, heap, 19);
    dst_range_sizes[1] = 4;
    src_handles[0] = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    src_range_sizes[0] = 0;
    src_handles[1] = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    src_range_sizes[1] = 0;
    ID3D12Device_CopyDescriptors(device, 2, dst_handles, dst_range_sizes,
            2, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12GraphicsCommandList_SetComputeRootSignature(command_list, context.root_signature);
    heaps[0] = sampler_heap; heaps[1] = heap;
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, ARRAY_SIZE(heaps), heaps);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 0,
            get_gpu_descriptor_handle(&context, heap, 5));
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 1,
            get_gpu_sampler_handle(&context, sampler_heap, 0));
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 2,
            get_gpu_descriptor_handle(&context, heap, 9));
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 3,
            get_gpu_descriptor_handle(&context, heap, 20));

    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_Dispatch(command_list, 1, 1, 1);

    transition_sub_resource_state(command_list, u[2], 0,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(u[2], DXGI_FORMAT_R32_UINT, &rb, queue, command_list);
    result = get_readback_data(&rb, 0, 0, 0, sizeof(*result));
    ok(result[ 0] == cb0_data, "Got unexpected value %#x.\n", result[0]);
    ok(result[ 1] == cb1_data, "Got unexpected value %#x.\n", result[1]);
    ok(result[ 2] == (unsigned int)cb2_data, "Got unexpected value %#x.\n", result[2]);
    ok(result[ 3] == 0, "Got unexpected value %#x.\n", result[3]);
    ok(result[ 4] == t0_data.x, "Got unexpected value %#x.\n", result[4]);
    ok(result[ 5] == t0_data.y, "Got unexpected value %#x.\n", result[5]);
    ok(result[ 6] == t0_data.z, "Got unexpected value %#x.\n", result[6]);
    ok(result[ 7] == t0_data.w, "Got unexpected value %#x.\n", result[7]);
    ok(result[ 8] == t0_data.x, "Got unexpected value %#x.\n", result[8]);
    ok(result[ 9] == t0_data.y, "Got unexpected value %#x.\n", result[9]);
    ok(result[10] == t0_data.z, "Got unexpected value %#x.\n", result[10]);
    ok(result[11] == t0_data.w, "Got unexpected value %#x.\n", result[11]);
    ok(result[12] == t0_data.x, "Got unexpected value %#x.\n", result[12]);
    ok(result[13] == t0_data.y, "Got unexpected value %#x.\n", result[13]);
    ok(result[14] == t0_data.z, "Got unexpected value %#x.\n", result[14]);
    ok(result[15] == t0_data.w, "Got unexpected value %#x.\n", result[15]);
    ok(result[16] == t1_data, "Got unexpected value %#x.\n", result[16]);
    ok(result[17] == (unsigned int)t2_data, "Got unexpected value %#x.\n", result[17]);
    ok(result[18] == (unsigned int)t3_data, "Got unexpected value %#x.\n", result[18]);
    ok(result[19] == (unsigned int)t4_data, "Got unexpected value %#x.\n", result[19]);
    ok(result[20] == t5_data.x, "Got unexpected value %#x.\n", result[20]);
    ok(result[21] == t5_data.y, "Got unexpected value %#x.\n", result[21]);
    ok(result[22] == t5_data.z, "Got unexpected value %#x.\n", result[22]);
    ok(result[23] == t5_data.w, "Got unexpected value %#x.\n", result[23]);
    ok(result[24] == 1, "Got unexpected value %#x.\n", result[24]);
    ok(result[25] == 1, "Got unexpected value %#x.\n", result[25]);
    ok(result[26] == 1, "Got unexpected value %#x.\n", result[26]);
    ok(result[27] == 1, "Got unexpected value %#x.\n", result[27]);
    ok(result[28] == 0, "Got unexpected value %#x.\n", result[28]);
    ok(result[29] == 0, "Got unexpected value %#x.\n", result[29]);
    ok(result[30] == 0, "Got unexpected value %#x.\n", result[30]);
    ok(result[31] == 0, "Got unexpected value %#x.\n", result[31]);
    ok(result[32] == u0_data.x, "Got unexpected value %#x.\n", result[32]);
    ok(result[33] == u0_data.y, "Got unexpected value %#x.\n", result[33]);
    ok(result[34] == u0_data.z, "Got unexpected value %#x.\n", result[34]);
    ok(result[35] == u0_data.w, "Got unexpected value %#x.\n", result[35]);
    ok(result[36] == u1_data.u[0], "Got unexpected value %#x.\n", result[36]);
    ok(result[37] == u1_data.u[1], "Got unexpected value %#x.\n", result[37]);
    ok(result[38] == u1_data.u[2], "Got unexpected value %#x.\n", result[38]);
    ok(result[39] == u1_data.f, "Got unexpected value %#x.\n", result[39]);
    ok(result[40] == u1_data.f, "Got unexpected value %#x.\n", result[40]);
    ok(result[41] == u1_data.f, "Got unexpected value %#x.\n", result[41]);
    ok(result[42] == u1_data.f, "Got unexpected value %#x.\n", result[42]);
    ok(result[43] == 0xdeadbeef, "Got unexpected value %#x.\n", result[43]);
    assert(rb.width == 44);
    release_resource_readback(&rb);

    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12DescriptorHeap_Release(cpu_sampler_heap);
    ID3D12DescriptorHeap_Release(cpu_sampler_heap2);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12DescriptorHeap_Release(sampler_heap);
    ID3D12Resource_Release(cb);
    for (i = 0; i < ARRAY_SIZE(t); ++i)
        ID3D12Resource_Release(t[i]);
    for (i = 0; i < ARRAY_SIZE(u); ++i)
        ID3D12Resource_Release(u[i]);
    destroy_depth_stencil(&ds);
    destroy_test_context(&context);
}

void test_copy_descriptors_range_sizes(void)
{
    D3D12_CPU_DESCRIPTOR_HANDLE dst_handles[1], src_handles[1];
    D3D12_CPU_DESCRIPTOR_HANDLE green_handle, blue_handle;
    ID3D12Resource *green_texture, *blue_texture;
    UINT dst_range_sizes[1], src_range_sizes[1];
    ID3D12GraphicsCommandList *command_list;
    ID3D12DescriptorHeap *cpu_heap;
    struct test_context_desc desc;
    D3D12_SUBRESOURCE_DATA data;
    struct resource_readback rb;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int i;
    D3D12_BOX box;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 32.0f;
            p.y = position.y / 32.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x7a0c3929, 0x75ff3ca4, 0xccb318b2, 0xe6965b4c, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000a4, 0x00000050,
        0x00000029, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002,
        0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const struct vec4 green = {0.0f, 1.0f, 0.0f, 1.0f};
    static const struct vec4 blue = {0.0f, 0.0f, 1.0f, 1.0f};

    memset(&desc, 0, sizeof(desc));
    desc.rt_width = desc.rt_height = 6;
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    cpu_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10);
    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8);

    green_handle = get_cpu_descriptor_handle(&context, cpu_heap, 0);
    blue_handle = get_cpu_descriptor_handle(&context, cpu_heap, 1);

    green_texture = create_default_texture(context.device,
            1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = &green;
    data.RowPitch = sizeof(green);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(green_texture, &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, green_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    ID3D12Device_CreateShaderResourceView(device, green_texture, NULL, green_handle);

    blue_texture = create_default_texture(context.device,
            1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = &blue;
    data.RowPitch = sizeof(blue);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(blue_texture, &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, blue_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    ID3D12Device_CreateShaderResourceView(device, blue_texture, NULL, blue_handle);

    context.root_signature = create_texture_root_signature(context.device,
            D3D12_SHADER_VISIBILITY_PIXEL, 0, 0);
    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps, NULL);

    /* copy descriptors */
    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 1);
    dst_range_sizes[0] = 1;
    src_handles[0] = blue_handle;
    src_range_sizes[0] = 1;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, dst_range_sizes,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 2);
    dst_range_sizes[0] = 1;
    src_handles[0] = green_handle;
    src_range_sizes[0] = 1;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, dst_range_sizes,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 3);
    src_handles[0] = blue_handle;
    src_range_sizes[0] = 1;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, NULL,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 4);
    src_handles[0] = green_handle;
    src_range_sizes[0] = 1;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, NULL,
            1, src_handles, src_range_sizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 5);
    src_handles[0] = blue_handle;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, NULL,
            1, src_handles, NULL, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dst_handles[0] = get_cpu_descriptor_handle(&context, heap, 0);
    src_handles[0] = green_handle;
    ID3D12Device_CopyDescriptors(device, 1, dst_handles, NULL,
            1, src_handles, NULL, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);

    for (i = 0; i < desc.rt_width; ++i)
    {
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
                get_gpu_descriptor_handle(&context, heap, i));
        set_viewport(&context.viewport, i, 0.0f, 1.0f, desc.rt_height, 0.0f, 1.0f);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);
    }

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    get_texture_readback_with_command_list(context.render_target, 0, &rb, queue, command_list);
    for (i = 0; i < desc.rt_width; ++i)
    {
        set_box(&box, i, 0, 0, i + 1, desc.rt_height, 1);
        check_readback_data_uint(&rb, &box, i % 2 ? 0xffff0000 : 0xff00ff00, 0);
    }
    release_resource_readback(&rb);

    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12Resource_Release(blue_texture);
    ID3D12Resource_Release(green_texture);
    destroy_test_context(&context);
}

void test_copy_rtv_descriptors(void)
{
    D3D12_CPU_DESCRIPTOR_HANDLE dst_ranges[1], src_ranges[2];
    ID3D12GraphicsCommandList *command_list;
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D12_HEAP_PROPERTIES heap_properties;
    UINT dst_sizes[1], src_sizes[2];
    ID3D12DescriptorHeap *rtv_heap;
    struct test_context_desc desc;
    struct test_context context;
    D3D12_RESOURCE_DESC rt_desc;
    ID3D12Resource *rt_texture;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    static const struct
    {
        float color[4];
    }
    clears[] =
    {
        {{1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 1.0f, 1.0f}},
    };

    static const UINT expected[] =
    {
        0xffffff00u,
        0xff0000ffu,
        0xff00ff00u,
        0xffff0000u,
    };

    memset(&desc, 0, sizeof(desc));
    desc.no_render_target = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    rt_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rt_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    rt_desc.Width = 1;
    rt_desc.Height = 1;
    rt_desc.DepthOrArraySize = 4;
    rt_desc.MipLevels = 1;
    rt_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rt_desc.SampleDesc.Count = 1;
    rt_desc.SampleDesc.Quality = 0;
    rt_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rt_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    hr = ID3D12Device_CreateCommittedResource(device,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &rt_desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, NULL,
            &IID_ID3D12Resource, (void **)&rt_texture);
    ok(hr == S_OK, "Failed to create committed resource, hr %#x.\n", hr);

    rtv_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 9);

    ID3D12Device_CreateRenderTargetView(device, rt_texture, NULL, get_cpu_rtv_handle(&context, rtv_heap, 0));
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, get_cpu_rtv_handle(&context, rtv_heap, 0), white, 0, NULL);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    rtv_desc.Texture2DArray.MipSlice = 0;
    rtv_desc.Texture2DArray.ArraySize = 1;
    rtv_desc.Texture2DArray.PlaneSlice = 0;

    for (i = 0; i < 4; i++)
    {
        rtv_desc.Texture2DArray.FirstArraySlice = i;
        ID3D12Device_CreateRenderTargetView(device, rt_texture, &rtv_desc,
                get_cpu_rtv_handle(&context, rtv_heap, 1 + i));
    }

    ID3D12Device_CopyDescriptorsSimple(device, 2,
            get_cpu_rtv_handle(&context, rtv_heap, 5),
            get_cpu_rtv_handle(&context, rtv_heap, 2),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    dst_ranges[0] = get_cpu_rtv_handle(&context, rtv_heap, 7);
    src_ranges[0] = get_cpu_rtv_handle(&context, rtv_heap, 4);
    src_ranges[1] = get_cpu_rtv_handle(&context, rtv_heap, 1);

    dst_sizes[0] = 2;
    src_sizes[0] = 1;
    src_sizes[1] = 1;

    ID3D12Device_CopyDescriptors(device,
            ARRAY_SIZE(dst_ranges), dst_ranges, dst_sizes,
            ARRAY_SIZE(src_ranges), src_ranges, src_sizes,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (i = 0; i < 4; i++)
        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, get_cpu_rtv_handle(&context, rtv_heap, 5 + i), clears[i].color, 0, NULL);

    for (i = 0; i < 4; i++)
    {
        transition_sub_resource_state(command_list, rt_texture, i,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        check_sub_resource_uint(rt_texture, i, queue, command_list, expected[i], 0);
        reset_command_list(command_list, context.allocator);
    }

    ID3D12DescriptorHeap_Release(rtv_heap);
    ID3D12Resource_Release(rt_texture);
    destroy_test_context(&context);
}

void test_descriptors_visibility(void)
{
    ID3D12Resource *vs_raw_buffer, *ps_raw_buffer;
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_ranges[2];
    D3D12_STATIC_SAMPLER_DESC sampler_desc[2];
    ID3D12Resource *vs_texture, *ps_texture;
    ID3D12GraphicsCommandList *command_list;
    D3D12_ROOT_PARAMETER root_parameters[6];
    ID3D12Resource *vs_cb, *ps_cb;
    struct test_context_desc desc;
    D3D12_SUBRESOURCE_DATA data;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        ByteAddressBuffer b;
        Texture2D t;
        SamplerState s;

        float4 cb;

        float4 main(uint id : SV_VertexID) : SV_Position
        {
            float2 coords = float2((id << 1) & 2, id & 2);
            uint i;

            if (cb.x != 4.0 || cb.y != 8.0 || cb.z != 16.0 || cb.w != 32.0)
                return (float4)0;

            for (i = 0; i <= 6; ++i)
            {
                if (b.Load(4 * i) != i)
                    return (float4)0;
            }

            if (any(t.SampleLevel(s, (float2)0, 0) != float4(1.0, 1.0, 0.0, 1.0)))
                return (float4)0;

            return float4(coords * float2(2, -2) + float2(-1, 1), 0, 1);
        }
#endif
        0x43425844, 0x046e4d13, 0xd2103a18, 0x8576703b, 0x6f58933a, 0x00000001, 0x0000043c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x565f5653, 0x65747265, 0x00444978,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69, 0x58454853, 0x000003a0, 0x00010050,
        0x000000e8, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000,
        0x00000000, 0x030000a1, 0x00107000, 0x00000000, 0x04001858, 0x00107000, 0x00000001, 0x00005555,
        0x04000060, 0x00101012, 0x00000000, 0x00000006, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x02000068, 0x00000002, 0x0b000039, 0x001000f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000000,
        0x00004002, 0x40800000, 0x41000000, 0x41800000, 0x42000000, 0x0700003c, 0x00100012, 0x00000000,
        0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010002a,
        0x00000000, 0x0010000a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010003a, 0x00000000,
        0x0010000a, 0x00000000, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0100003e, 0x01000015, 0x05000036,
        0x00100012, 0x00000000, 0x00004001, 0x00000000, 0x01000030, 0x0700004f, 0x00100022, 0x00000000,
        0x00004001, 0x00000006, 0x0010000a, 0x00000000, 0x03040003, 0x0010001a, 0x00000000, 0x07000029,
        0x00100022, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x890000a5, 0x800002c2,
        0x00199983, 0x00100022, 0x00000000, 0x0010001a, 0x00000000, 0x00107006, 0x00000000, 0x07000027,
        0x00100022, 0x00000000, 0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0304001f, 0x0010001a,
        0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x0100003e, 0x01000015, 0x0700001e, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
        0x00004001, 0x00000001, 0x01000016, 0x90000048, 0x800000c2, 0x00155543, 0x001000f2, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00107e46, 0x00000001, 0x00106000,
        0x00000000, 0x00004001, 0x00000000, 0x0a000039, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00004002, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x0700003c, 0x00100032, 0x00000000,
        0x00100ae6, 0x00000000, 0x00100046, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010001a,
        0x00000000, 0x0010000a, 0x00000000, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2,
        0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0100003e, 0x01000015,
        0x0b00008c, 0x00100012, 0x00000000, 0x00004001, 0x00000001, 0x00004001, 0x00000001, 0x0010100a,
        0x00000000, 0x00004001, 0x00000000, 0x07000001, 0x00100022, 0x00000000, 0x0010100a, 0x00000000,
        0x00004001, 0x00000002, 0x05000056, 0x00100032, 0x00000001, 0x00100046, 0x00000000, 0x0f000032,
        0x00102032, 0x00000000, 0x00100046, 0x00000001, 0x00004002, 0x40000000, 0xc0000000, 0x00000000,
        0x00000000, 0x00004002, 0xbf800000, 0x3f800000, 0x00000000, 0x00000000, 0x08000036, 0x001020c2,
        0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE vs = {vs_code, sizeof(vs_code)};
    static const DWORD ps_code[] =
    {
#if 0
        ByteAddressBuffer b;
        Texture2D t;
        SamplerState s;

        float4 cb;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            if (cb.x != 1.0 || cb.y != 2.0 || cb.z != 3.0 || cb.w != 4.0)
                return float4(1.0, 0.0, 0.0, 1.0);

            if (b.Load(0) != 2 || b.Load(4) != 4 || b.Load(8) != 8)
                return float4(1.0, 0.0, 0.0, 1.0);

            return t.Sample(s, float2(position.x / 32.0, position.y / 32.0));
        }
#endif
        0x43425844, 0x1b1aafc1, 0xeab215f6, 0x77d65b25, 0x03cbe695, 0x00000001, 0x000002dc, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000240, 0x00000050,
        0x00000090, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000,
        0x00000000, 0x030000a1, 0x00107000, 0x00000000, 0x04001858, 0x00107000, 0x00000001, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0b000039, 0x001000f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000000, 0x00004002,
        0x3f800000, 0x40000000, 0x40400000, 0x40800000, 0x0700003c, 0x00100012, 0x00000000, 0x0010001a,
        0x00000000, 0x0010000a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010002a, 0x00000000,
        0x0010000a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010003a, 0x00000000, 0x0010000a,
        0x00000000, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x890000a5, 0x800002c2,
        0x00199983, 0x00100072, 0x00000000, 0x00004001, 0x00000000, 0x00107246, 0x00000000, 0x0a000027,
        0x00100072, 0x00000000, 0x00100246, 0x00000000, 0x00004002, 0x00000002, 0x00000004, 0x00000008,
        0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010001a, 0x00000000, 0x0010000a, 0x00000000,
        0x0700003c, 0x00100012, 0x00000000, 0x0010002a, 0x00000000, 0x0010000a, 0x00000000, 0x0304001f,
        0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x3f800000, 0x00000000,
        0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x0a000038, 0x00100032, 0x00000000, 0x00101046,
        0x00000000, 0x00004002, 0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2,
        0x00155543, 0x001000f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000001, 0x00106000,
        0x00000000, 0x05000036, 0x001020f2, 0x00000000, 0x00100e46, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const struct vec4 vs_cb_data = {4.0f, 8.0f, 16.0f, 32.0f};
    static const struct vec4 ps_cb_data = {1.0f, 2.0f, 3.0f, 4.0f};
    static const uint32_t vs_buffer_data[] = {0, 1, 2, 3, 4, 5, 6};
    static const uint32_t ps_buffer_data[] = {2, 4, 8};
    static const float vs_texture_data[] = {1.0f, 1.0f, 0.0f, 1.0f};
    static const float ps_texture_data[] = {0.0f, 1.0f, 0.0f, 1.0f};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    sampler_desc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc[0].MipLODBias = 0.0f;
    sampler_desc[0].MaxAnisotropy = 0;
    sampler_desc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler_desc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler_desc[0].MinLOD = 0.0f;
    sampler_desc[0].MaxLOD = 0.0f;
    sampler_desc[0].ShaderRegister = 0;
    sampler_desc[0].RegisterSpace = 0;
    sampler_desc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    sampler_desc[1] = sampler_desc[0];
    sampler_desc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[0].Descriptor.ShaderRegister = 0;
    root_parameters[0].Descriptor.RegisterSpace = 0;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[1].Descriptor.ShaderRegister = 0;
    root_parameters[1].Descriptor.RegisterSpace = 0;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_parameters[2].Descriptor.ShaderRegister = 0;
    root_parameters[2].Descriptor.RegisterSpace = 0;
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_parameters[3].Descriptor.ShaderRegister = 0;
    root_parameters[3].Descriptor.RegisterSpace = 0;
    root_parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[0].NumDescriptors = 1;
    descriptor_ranges[0].BaseShaderRegister = 1;
    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[4].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[4].DescriptorTable.pDescriptorRanges = &descriptor_ranges[0];
    root_parameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[1].NumDescriptors = 1;
    descriptor_ranges[1].BaseShaderRegister = 1;
    descriptor_ranges[1].RegisterSpace = 0;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    root_parameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[5].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[5].DescriptorTable.pDescriptorRanges = &descriptor_ranges[1];
    root_parameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = 6;
    root_signature_desc.pParameters = root_parameters;
    root_signature_desc.NumStaticSamplers = 2;
    root_signature_desc.pStaticSamplers = sampler_desc;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_pipeline_state(device,
            context.root_signature, context.render_target_desc.Format,
            &vs, &ps, NULL);

    vs_cb = create_upload_buffer(device, sizeof(vs_cb_data), &vs_cb_data);
    ps_cb = create_upload_buffer(device, sizeof(ps_cb_data), &ps_cb_data);

    vs_raw_buffer = create_upload_buffer(device, sizeof(vs_buffer_data), vs_buffer_data);
    ps_raw_buffer = create_upload_buffer(device, sizeof(ps_buffer_data), ps_buffer_data);

    vs_texture = create_default_texture(device,
            1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = vs_texture_data;
    data.RowPitch = sizeof(vs_texture_data);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(vs_texture, &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, vs_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    ps_texture = create_default_texture(device,
            1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    data.pData = ps_texture_data;
    data.RowPitch = sizeof(ps_texture_data);
    data.SlicePitch = data.RowPitch;
    upload_texture_data(ps_texture, &data, 1, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, ps_texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
    ID3D12Device_CreateShaderResourceView(device, vs_texture, NULL,
            get_cpu_descriptor_handle(&context, heap, 0));
    ID3D12Device_CreateShaderResourceView(device, ps_texture, NULL,
            get_cpu_descriptor_handle(&context, heap, 1));

    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(command_list,
            0, ID3D12Resource_GetGPUVirtualAddress(vs_cb));
    ID3D12GraphicsCommandList_SetGraphicsRootConstantBufferView(command_list,
            1, ID3D12Resource_GetGPUVirtualAddress(ps_cb));
    ID3D12GraphicsCommandList_SetGraphicsRootShaderResourceView(command_list,
            2, ID3D12Resource_GetGPUVirtualAddress(vs_raw_buffer));
    ID3D12GraphicsCommandList_SetGraphicsRootShaderResourceView(command_list,
            3, ID3D12Resource_GetGPUVirtualAddress(ps_raw_buffer));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list,
            4, get_gpu_descriptor_handle(&context, heap, 0));
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list,
            5, get_gpu_descriptor_handle(&context, heap, 1));

    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_resource_state(command_list, context.render_target,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xff00ff00, 0);

    ID3D12Resource_Release(vs_cb);
    ID3D12Resource_Release(ps_cb);
    ID3D12Resource_Release(vs_texture);
    ID3D12Resource_Release(ps_texture);
    ID3D12Resource_Release(vs_raw_buffer);
    ID3D12Resource_Release(ps_raw_buffer);
    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_create_null_descriptors(void)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12Device *device;
    HRESULT hr;

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 16;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc,
            &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(SUCCEEDED(hr), "Failed to create descriptor heap, hr %#x.\n", hr);

    cbv_desc.BufferLocation = 0;
    cbv_desc.SizeInBytes = 0;
    ID3D12Device_CreateConstantBufferView(device, &cbv_desc,
            get_cpu_descriptor_handle(&context, heap, 0));

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = 1;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    ID3D12Device_CreateShaderResourceView(device, NULL, &srv_desc,
            get_cpu_descriptor_handle(&context, heap, 1));

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MipLevels = 1;
    ID3D12Device_CreateShaderResourceView(device, NULL, &srv_desc,
            get_cpu_descriptor_handle(&context, heap, 2));

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_UINT;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = 1;
    ID3D12Device_CreateUnorderedAccessView(device, NULL, NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, heap, 3));

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.Format = DXGI_FORMAT_R32_UINT;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    uav_desc.Texture2D.PlaneSlice = 0;
    ID3D12Device_CreateUnorderedAccessView(device, NULL, NULL, &uav_desc,
            get_cpu_descriptor_handle(&context, heap, 3));

    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_null_cbv(void)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    ID3D12GraphicsCommandList *command_list;
    D3D12_ROOT_PARAMETER root_parameters[2];
    D3D12_DESCRIPTOR_RANGE descriptor_range;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int index;
    HRESULT hr;

    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const DWORD ps_code[] =
    {
#if 0
        uint index;

        cbuffer null_cb
        {
            float4 data[1024];
        };

        float4 main() : SV_Target
        {
            return data[index];
        }
#endif
        0x43425844, 0xa69026e2, 0xccf934be, 0x11f0a922, 0x95e9ab51, 0x00000001, 0x000000f0, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000078, 0x00000050, 0x0000001e,
        0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04000859, 0x00208e46, 0x00000001,
        0x00000400, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x06000036, 0x00100012,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x07000036, 0x001020f2, 0x00000000, 0x04208e46,
        0x00000001, 0x0010000a, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;
    device = context.device;

    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptor_range.NumDescriptors = 1;
    descriptor_range.BaseShaderRegister = 1;
    descriptor_range.RegisterSpace = 0;
    descriptor_range.OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_range;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[1].Constants.ShaderRegister = 0;
    root_parameters[1].Constants.RegisterSpace = 0;
    root_parameters[1].Constants.Num32BitValues = 1;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    memset(&root_signature_desc, 0, sizeof(root_signature_desc));
    root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters);
    root_signature_desc.pParameters = root_parameters;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);

    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps, NULL);

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

    cbv_desc.BufferLocation = 0;
    cbv_desc.SizeInBytes = 0; /* Size doesn't appear to matter for NULL CBV. */
    ID3D12Device_CreateConstantBufferView(device, &cbv_desc,
            ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

    for (index = 0; index < 1200; index += 100)
    {
        vkd3d_test_set_context("index %u", index);

        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

        ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
        ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
        ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
        ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
                ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
        ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 1, 1, &index, 0);
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

        transition_sub_resource_state(command_list, context.render_target, 0,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        check_sub_resource_uint(context.render_target, 0, queue, command_list, 0x00000000, 0);

        reset_command_list(command_list, context.allocator);
        transition_sub_resource_state(command_list, context.render_target, 0,
                D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    vkd3d_test_set_context(NULL);

    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_null_srv(void)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D12GraphicsCommandList *command_list;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    struct uvec4 location;
    ID3D12Device *device;
    unsigned int i, j;

    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const DWORD ps_sample_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return t.Sample(s, float2(position.x / 32.0f, position.y / 32.0f));
        }
#endif
        0x43425844, 0xe096fa11, 0xeb01c081, 0x961588d4, 0x27c031af, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000a4, 0x00000050,
        0x00000029, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002,
        0x3d000000, 0x3d000000, 0x00000000, 0x00000000, 0x8b000045, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_sample = {ps_sample_code, sizeof(ps_sample_code)};
    static const DWORD ps_ld_code[] =
    {
#if 0
        Texture2D t;

        uint4 location;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return t.Load(location.xyz);
        }
#endif
        0x43425844, 0xfa13670e, 0x291af510, 0xc253cc12, 0x9474950b, 0x00000001, 0x00000100, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000064, 0x00000050,
        0x00000019, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04001858, 0x00107000,
        0x00000000, 0x00005555, 0x03000065, 0x001020f2, 0x00000000, 0x8a00002d, 0x800000c2, 0x00155543,
        0x001020f2, 0x00000000, 0x00208a46, 0x00000000, 0x00000000, 0x00107e46, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_ld = {ps_ld_code, sizeof(ps_ld_code)};
    static const DWORD ps_buffer_code[] =
    {
#if 0
        ByteAddressBuffer t;

        uint location;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return t.Load(location);
        }
#endif
        0x43425844, 0x70170f6b, 0x16097169, 0x714f155c, 0x1e3d860f, 0x00000001, 0x00000118, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x0000007c, 0x00000050,
        0x0000001f, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x030000a1, 0x00107000,
        0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x8a0000a5, 0x800002c2,
        0x00199983, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x00107006, 0x00000000,
        0x05000056, 0x001020f2, 0x00000000, 0x00100006, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_buffer = {ps_buffer_code, sizeof(ps_buffer_code)};
    static const DXGI_FORMAT formats[] =
    {
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
    };
    /* component mapping is ignored for NULL SRVs */
    static const unsigned int component_mappings[] =
    {
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1,
                D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1,
                D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1,
                D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1),
    };

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;
    device = context.device;

    context.root_signature = create_texture_root_signature(context.device,
            D3D12_SHADER_VISIBILITY_PIXEL, 4, 0);

    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps_sample, NULL);

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(component_mappings); ++j)
        {
            vkd3d_test_set_context("format %#x, component mapping %#x", formats[i], component_mappings[j]);

            memset(&srv_desc, 0, sizeof(srv_desc));
            srv_desc.Format = formats[i];
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Shader4ComponentMapping = component_mappings[j];
            srv_desc.Texture2D.MipLevels = 1;
            ID3D12Device_CreateShaderResourceView(device, NULL, &srv_desc,
                    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

            ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

            ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
            ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
            ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
            ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
            ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
                    ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
            ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
            ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
            ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

            transition_sub_resource_state(command_list, context.render_target, 0,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
            check_sub_resource_uint(context.render_target, 0, queue, command_list, 0x00000000, 0);

            reset_command_list(command_list, context.allocator);
            transition_sub_resource_state(command_list, context.render_target, 0,
                    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
    }
    vkd3d_test_set_context(NULL);

    ID3D12PipelineState_Release(context.pipeline_state);
    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps_ld, NULL);

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MipLevels = 1;
    ID3D12Device_CreateShaderResourceView(device, NULL, &srv_desc,
            ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    location.x = 10;
    location.y = 20;
    location.z = 0;
    location.w = 0;
    ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 1, 4, &location, 0);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, context.render_target, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0x00000000, 0);

    /* buffer */
    ID3D12PipelineState_Release(context.pipeline_state);
    context.pipeline_state = create_pipeline_state(context.device,
            context.root_signature, context.render_target_desc.Format, NULL, &ps_buffer, NULL);
    reset_command_list(command_list, context.allocator);
    transition_sub_resource_state(command_list, context.render_target, 0,
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = 1024;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    ID3D12Device_CreateShaderResourceView(device, NULL, &srv_desc,
            ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
    ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
            ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    location.x = 0;
    ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 1, 4, &location, 0);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, context.render_target, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0x00000000, 0);

    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_null_uav(void)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_ranges[1];
    D3D12_ROOT_PARAMETER root_parameters[2];
    const D3D12_SHADER_BYTECODE *current_ps;
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    ID3D12DescriptorHeap *uav_heap;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const DWORD ps_ld_texture_code[] =
    {
#if 0
        RWTexture2D<float> u;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            float2 s;
            u.GetDimensions(s.x, s.y);
            return u[s * float2(position.x / 640.0f, position.y / 480.0f)];
        }
#endif
        0x43425844, 0x85c096ab, 0x210d7572, 0xdb1951af, 0x4dadced7, 0x00000001, 0x00000194, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000f8, 0x00000050,
        0x0000003e, 0x0100086a, 0x0400189c, 0x0011e000, 0x00000001, 0x00005555, 0x04002064, 0x00101032,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x8900003d,
        0x800000c2, 0x00155543, 0x00100032, 0x00000000, 0x00004001, 0x00000000, 0x0011ee46, 0x00000001,
        0x07000038, 0x001000f2, 0x00000000, 0x00100546, 0x00000000, 0x00101546, 0x00000000, 0x0a000038,
        0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x3b088889,
        0x3b088889, 0x0500001c, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x890000a3, 0x800000c2,
        0x00155543, 0x00100012, 0x00000000, 0x00100e46, 0x00000000, 0x0011ee46, 0x00000001, 0x05000036,
        0x001020f2, 0x00000000, 0x00100006, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_ld_texture = {ps_ld_texture_code, sizeof(ps_ld_texture_code)};
    static const DWORD ps_ld_buffer_code[] =
    {
#if 0
        RWByteAddressBuffer u;

        uint location;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return u.Load(4 * location);
        }
#endif
        0x43425844, 0xde636789, 0x7bc99233, 0x8b0609b6, 0x4b9a958e, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000098, 0x00000050,
        0x00000026, 0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300009d, 0x0011e000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x08000029, 0x00100012,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x00004001, 0x00000002, 0x890000a5, 0x800002c2,
        0x00199983, 0x00100012, 0x00000000, 0x0010000a, 0x00000000, 0x0011e006, 0x00000001, 0x05000056,
        0x001020f2, 0x00000000, 0x00100006, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_ld_buffer = {ps_ld_buffer_code, sizeof(ps_ld_buffer_code)};
    static const struct test
    {
        const D3D12_SHADER_BYTECODE *ps;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
        uint32_t location;
    }
    tests[] =
    {
        {&ps_ld_texture, {DXGI_FORMAT_R32_UINT, D3D12_UAV_DIMENSION_TEXTURE2D}, 0},
        {&ps_ld_buffer,
                {DXGI_FORMAT_R32_TYPELESS, D3D12_UAV_DIMENSION_BUFFER, .Buffer = {0, 1024, .Flags = D3D12_BUFFER_UAV_FLAG_RAW}},
                0},
        {&ps_ld_buffer,
                {DXGI_FORMAT_R32_TYPELESS, D3D12_UAV_DIMENSION_BUFFER, .Buffer = {0, 1024, .Flags = D3D12_BUFFER_UAV_FLAG_RAW}},
                1024},
    };

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[0].NumDescriptors = 1;
    descriptor_ranges[0].BaseShaderRegister = 1;
    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = descriptor_ranges;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[1].Constants.ShaderRegister = 0;
    root_parameters[1].Constants.RegisterSpace = 0;
    root_parameters[1].Constants.Num32BitValues = 1;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters);
    root_signature_desc.pParameters = root_parameters;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = NULL;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    hr = create_root_signature(device, &root_signature_desc, &context.root_signature);
    ok(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);

    uav_heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    current_ps = NULL;
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const struct test *test = &tests[i];

        vkd3d_test_set_context("Test %u", i);

        if (current_ps != test->ps)
        {
            if (context.pipeline_state)
                ID3D12PipelineState_Release(context.pipeline_state);
            current_ps = tests[i].ps;
            context.pipeline_state = create_pipeline_state(context.device,
                    context.root_signature, context.render_target_desc.Format, NULL, current_ps, NULL);
        }

        cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(uav_heap);
        ID3D12Device_CreateUnorderedAccessView(device, NULL, NULL, &test->uav_desc, cpu_handle);

        ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
        ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &uav_heap);
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0,
                ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(uav_heap));
        ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstant(command_list, 1, test->location, 0);
        ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
        ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

        transition_sub_resource_state(command_list, context.render_target, 0,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        check_sub_resource_uint(context.render_target, 0, queue, command_list, 0x00000000, 0);

        reset_command_list(command_list, context.allocator);
        transition_sub_resource_state(command_list, context.render_target, 0,
                D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    vkd3d_test_set_context(NULL);

    ID3D12DescriptorHeap_Release(uav_heap);
    destroy_test_context(&context);
}

void test_null_rtv(void)
{
    ID3D12GraphicsCommandList *command_list;
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D12_HEAP_PROPERTIES heap_properties;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;

    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    memset(&desc, 0, sizeof(desc));
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv_desc.Texture2D.MipSlice = 0;
    rtv_desc.Texture2D.PlaneSlice = 0;
    ID3D12Device_CreateRenderTargetView(device, context.render_target, &rtv_desc, context.rtv);
    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, true, NULL);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, white, 0, NULL);

    ID3D12Device_CreateRenderTargetView(device, NULL, &rtv_desc, context.rtv);
    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, true, NULL);

    /* Attempting to clear a NULL RTV crashes on native D3D12, so try to draw something instead */
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, context.render_target, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(context.render_target, 0, queue, command_list, 0xffffffff, 0);

    destroy_test_context(&context);
}

void test_cpu_descriptors_lifetime(void)
{
    static const float blue[] = {0.0f, 0.0f, 1.0f, 1.0f};
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    ID3D12Resource *red_resource, *blue_resource;
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC resource_desc;
    ID3D12DescriptorHeap *rtv_heap;
    D3D12_CLEAR_VALUE clear_value;
    struct test_context context;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    HRESULT hr;

    if (!init_test_context(&context, NULL))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    rtv_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
    rtv_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtv_heap);

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = 32;
    resource_desc.Height = 32;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clear_value.Color[0] = 1.0f;
    clear_value.Color[1] = 0.0f;
    clear_value.Color[2] = 0.0f;
    clear_value.Color[3] = 1.0f;
    hr = ID3D12Device_CreateCommittedResource(device,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
            &IID_ID3D12Resource, (void **)&red_resource);
    ok(hr == S_OK, "Failed to create texture, hr %#x.\n", hr);
    clear_value.Color[0] = 0.0f;
    clear_value.Color[1] = 0.0f;
    clear_value.Color[2] = 1.0f;
    clear_value.Color[3] = 1.0f;
    hr = ID3D12Device_CreateCommittedResource(device,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
            &IID_ID3D12Resource, (void **)&blue_resource);
    ok(hr == S_OK, "Failed to create texture, hr %#x.\n", hr);

    ID3D12Device_CreateRenderTargetView(device, red_resource, NULL, rtv_handle);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv_handle, red, 0, NULL);
    /* Destroy the previous RTV and create a new one in its place. */
    ID3D12Device_CreateRenderTargetView(device, blue_resource, NULL, rtv_handle);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv_handle, blue, 0, NULL);

    /* Destroy the CPU descriptor heap. */
    ID3D12DescriptorHeap_Release(rtv_heap);

    transition_resource_state(command_list, red_resource,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition_resource_state(command_list, blue_resource,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(red_resource, 0, queue, command_list, 0xff0000ff, 0);
    reset_command_list(command_list, context.allocator);
    check_sub_resource_uint(blue_resource, 0, queue, command_list, 0xffff0000, 0);

    rtv_heap = create_cpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
    rtv_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtv_heap);

    /* Try again with OMSetRenderTargets(). */
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, red_resource,
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    transition_resource_state(command_list, blue_resource,
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    ID3D12Device_CreateRenderTargetView(device, red_resource, NULL, rtv_handle);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv_handle, red, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &rtv_handle, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    /* Destroy the previous RTV and create a new one in its place. */
    ID3D12Device_CreateRenderTargetView(device, blue_resource, NULL, rtv_handle);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv_handle, blue, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &rtv_handle, false, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    /* Destroy the previous RTV and create a new one in its place. */
    ID3D12Device_CreateRenderTargetView(device, red_resource, NULL, rtv_handle);

    /* Destroy the CPU descriptor heap. */
    ID3D12DescriptorHeap_Release(rtv_heap);

    transition_resource_state(command_list, red_resource,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition_resource_state(command_list, blue_resource,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(red_resource, 0, queue, command_list, 0xff00ff00, 0);
    reset_command_list(command_list, context.allocator);
    check_sub_resource_uint(blue_resource, 0, queue, command_list, 0xff00ff00, 0);

    ID3D12Resource_Release(blue_resource);
    ID3D12Resource_Release(red_resource);
    destroy_test_context(&context);
}

void test_create_sampler(void)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    unsigned int sampler_increment_size;
    D3D12_SAMPLER_DESC sampler_desc;
    ID3D12DescriptorHeap *heap;
    ID3D12Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    sampler_increment_size = ID3D12Device_GetDescriptorHandleIncrementSize(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    trace("Sampler descriptor handle increment size: %u.\n", sampler_increment_size);
    ok(sampler_increment_size, "Got unexpected increment size %#x.\n", sampler_increment_size);

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.NumDescriptors = 16;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(SUCCEEDED(hr), "Failed to create descriptor heap, hr %#x.\n", hr);

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    ID3D12Device_CreateSampler(device, &sampler_desc, cpu_handle);

    cpu_handle.ptr += sampler_increment_size;
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    for (i = 1; i < heap_desc.NumDescriptors; ++i)
    {
        ID3D12Device_CreateSampler(device, &sampler_desc, cpu_handle);
        cpu_handle.ptr += sampler_increment_size;
    }

    trace("MinMaxFiltering: %#x.\n", is_min_max_filtering_supported(device));
    if (is_min_max_filtering_supported(device))
    {
        cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
        sampler_desc.Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        ID3D12Device_CreateSampler(device, &sampler_desc, cpu_handle);

        cpu_handle.ptr += sampler_increment_size;
        sampler_desc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        ID3D12Device_CreateSampler(device, &sampler_desc, cpu_handle);
    }

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
    ID3D12Device_CreateSampler(device, &sampler_desc, cpu_handle);

    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

void test_create_sampler2(void)
{
    /* This is just a smoke test that calling CreateSampler2() does not blow up.
     * Also, note that there is no CreateSampler1(). */
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    D3D12_SAMPLER_DESC2 sampler_desc;
    ID3D12DescriptorHeap *heap;
    ID3D12Device11 *device11;
    ID3D12Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    if (FAILED(ID3D12Device_QueryInterface(device, &IID_ID3D12Device11, (void**)&device11)))
    {
        skip("ID3D12Device11 not supported.\n");
        ID3D12Device_Release(device);
        return;
    }

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.NumDescriptors = 16;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&heap);
    ok(SUCCEEDED(hr), "Failed to create descriptor heap, hr %#x.\n", hr);

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
    ID3D12Device11_CreateSampler2(device11, &sampler_desc, cpu_handle);

    ID3D12Device11_Release(device11);

    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

void test_create_unordered_access_view(void)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    ID3D12Resource *buffer, *texture;
    unsigned int descriptor_size;
    ID3D12DescriptorHeap *heap;
    ID3D12Device *device;
    ULONG refcount;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    trace("CBV/SRV/UAV descriptor size: %u.\n", descriptor_size);
    ok(descriptor_size, "Got unexpected descriptor size %#x.\n", descriptor_size);

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

    buffer = create_default_buffer(device, 64 * sizeof(float),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = 64;
    uav_desc.Buffer.StructureByteStride = 0;
    uav_desc.Buffer.CounterOffsetInBytes = 0;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    ID3D12Device_CreateUnorderedAccessView(device, buffer, NULL, &uav_desc, cpu_handle);

    cpu_handle.ptr += descriptor_size;

    /* DXGI_FORMAT_R32_UINT view for DXGI_FORMAT_R8G8B8A8_TYPELESS resources. */
    texture = create_default_texture(device, 8, 8, DXGI_FORMAT_R8G8B8A8_TYPELESS,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    uav_desc.Format = DXGI_FORMAT_R32_UINT;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;
    uav_desc.Texture2D.PlaneSlice = 0;
    ID3D12Device_CreateUnorderedAccessView(device, texture, NULL, &uav_desc, cpu_handle);

    ID3D12Resource_Release(buffer);
    ID3D12Resource_Release(texture);
    refcount = ID3D12DescriptorHeap_Release(heap);
    ok(!refcount, "ID3D12DescriptorHeap has %u references left.\n", (unsigned int)refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

void test_sampler_border_color(void)
{
    ID3D12DescriptorHeap *heap, *sampler_heap, *heaps[2];
    D3D12_STATIC_SAMPLER_DESC static_sampler_desc;
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_range[2];
    D3D12_ROOT_PARAMETER root_parameters[2];
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    ID3D12RootSignature *root_signature;
    ID3D12PipelineState *pipeline_state;
    D3D12_SAMPLER_DESC sampler_desc;
    struct test_context_desc desc;
    struct resource_readback rb;
    struct test_context context;
    ID3D12CommandQueue *queue;
    ID3D12Resource *texture;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return t.Sample(s, float2(-0.5f, 1.5f));
        }
#endif
        0x43425844, 0xf3ecc2e5, 0x82cfcce7, 0x1adcaaac, 0x9a0d8de0, 0x00000001, 0x0000010c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000070, 0x00000050,
        0x0000001c, 0x0100086a, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x03000065, 0x001020f2, 0x00000000, 0x8e000045, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00004002, 0xbf000000, 0x3fc00000, 0x00000000, 0x00000000, 0x00107e46, 0x00000000,
        0x00106000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const struct
    {
        bool static_sampler;
        unsigned int expected_color;
        float border_color[4];
        D3D12_STATIC_BORDER_COLOR static_border_color;
    }
    tests[] =
    {
        {false, 0x00000000u, {0.0f, 0.0f, 0.0f, 0.0f}},
        {false, 0xff000000u, {0.0f, 0.0f, 0.0f, 1.0f}},
        {false, 0xffffffffu, {1.0f, 1.0f, 1.0f, 1.0f}},
        {false, 0xccb3804du, {0.3f, 0.5f, 0.7f, 0.8f}},
        {true,  0x00000000u, {0.0f, 0.0f, 0.0f, 0.0f}, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK},
        {true,  0xff000000u, {0.0f, 0.0f, 0.0f, 1.0f}, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK},
        {true,  0xffffffffu, {1.0f, 1.0f, 1.0f, 1.0f}, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE},
    };

    memset(&desc, 0, sizeof(desc));
    desc.rt_width = 640;
    desc.rt_height = 480;
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list;
    queue = context.queue;

    descriptor_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_range[0].NumDescriptors = 1;
    descriptor_range[0].BaseShaderRegister = 0;
    descriptor_range[0].RegisterSpace = 0;
    descriptor_range[0].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[0].DescriptorTable.pDescriptorRanges = &descriptor_range[0];
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptor_range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    descriptor_range[1].NumDescriptors = 1;
    descriptor_range[1].BaseShaderRegister = 0;
    descriptor_range[1].RegisterSpace = 0;
    descriptor_range[1].OffsetInDescriptorsFromTableStart = 0;
    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    root_parameters[1].DescriptorTable.pDescriptorRanges = &descriptor_range[1];
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    cpu_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    gpu_handle = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap);

    sampler_heap = create_gpu_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);

    {
        D3D12_SUBRESOURCE_DATA sub;
        sub.pData = red;
        sub.RowPitch = 4;
        sub.SlicePitch = 4;
        texture = create_default_texture2d(device, 1, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
                D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
        upload_texture_data(texture, &sub, 1, queue, command_list);
        reset_command_list(command_list, context.allocator);
        transition_resource_state(command_list, texture,
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        ID3D12Device_CreateShaderResourceView(device, texture, NULL, cpu_handle);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        unsigned int color;
        vkd3d_test_set_context("Test %u", i);

        memset(&root_signature_desc, 0, sizeof(root_signature_desc));
        root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters);
        root_signature_desc.pParameters = root_parameters;

        if (tests[i].static_sampler)
        {
            root_signature_desc.NumParameters -= 1;
            root_signature_desc.NumStaticSamplers = 1;
            root_signature_desc.pStaticSamplers = &static_sampler_desc;

            memset(&static_sampler_desc, 0, sizeof(static_sampler_desc));
            static_sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            static_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            static_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            static_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            static_sampler_desc.BorderColor = tests[i].static_border_color;
            static_sampler_desc.ShaderRegister = 0;
            static_sampler_desc.RegisterSpace = 0;
            static_sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }
        else
        {
            memset(&sampler_desc, 0, sizeof(sampler_desc));
            sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            memcpy(sampler_desc.BorderColor, tests[i].border_color, sizeof(sampler_desc.BorderColor));
            ID3D12Device_CreateSampler(device, &sampler_desc, get_cpu_sampler_handle(&context, sampler_heap, 0));
        }

        hr = create_root_signature(device, &root_signature_desc, &root_signature);
        ok(hr == S_OK, "Failed to create root signature, hr %#x.\n", hr);

        pipeline_state = create_pipeline_state(device, root_signature,
                context.render_target_desc.Format, NULL, &ps, NULL);

        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, red, 0, NULL);

        ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
        ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, root_signature);
        ID3D12GraphicsCommandList_SetPipelineState(command_list, pipeline_state);
        heaps[0] = heap;
        heaps[1] = sampler_heap;
        ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, ARRAY_SIZE(heaps), heaps);
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0, gpu_handle);
        if (!tests[i].static_sampler)
        {
            ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 1,
                    get_gpu_sampler_handle(&context, sampler_heap, 0));
        }
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

        transition_resource_state(command_list, context.render_target,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

        get_texture_readback_with_command_list(context.render_target, 0, &rb, queue, command_list);

        color = get_readback_uint(&rb, 0, 0, 0);
        ok(compare_color(color, tests[i].expected_color, 1),
                "Got color 0x%08x, expected 0x%08x.\n",
                color, tests[i].expected_color);

        release_resource_readback(&rb);

        ID3D12RootSignature_Release(root_signature);
        ID3D12PipelineState_Release(pipeline_state);

        reset_command_list(command_list, context.allocator);
        transition_resource_state(command_list, context.render_target,
                D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    vkd3d_test_set_context(NULL);

    ID3D12Resource_Release(texture);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12DescriptorHeap_Release(sampler_heap);
    destroy_test_context(&context);
}

static void test_typed_buffers_many_objects(bool use_dxil)
{
    ID3D12DescriptorHeap *cpu_heap, *gpu_heap;
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    D3D12_DESCRIPTOR_RANGE descriptor_ranges[2];
    D3D12_ROOT_PARAMETER root_parameters[2];

    ID3D12Resource *output_buffer, *input_buffer;
    struct resource_readback rb;

    unsigned int i, j, descriptor_size;
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE host_handle;
    D3D12_CPU_DESCRIPTOR_HANDLE visible_handle;
    struct test_context context;
    ID3D12CommandQueue *queue;
    HRESULT hr;

#if 0
    RWBuffer<uint> RWBufs[] : register(u0);
    Buffer<uint> Bufs[] : register(t0);

    cbuffer Params : register(b0) {
      uint first;
    }

    [numthreads(1, 1, 1)]
    void main(uint tid : SV_DispatchThreadID)
    {
        uint idx = first + tid;
        uint val;
        InterlockedAdd(RWBufs[idx][1], 200, val);
        uint dummy_val;
        // This will be masked, except for the first iteration, which has a large view.
        InterlockedAdd(RWBufs[idx][2], 400, dummy_val);

        uint size;
        RWBufs[idx].GetDimensions(size);
        uint ro_size;
        Bufs[idx].GetDimensions(ro_size);

        uint success = 0;

        if (idx == 0)
        {
            if (size == 16 * 1024 * 1024)
                success |= 0x10;
            if (ro_size == 16 * 1024 * 1024)
                success |= 0x20;
        }
        else
        {
            if (size == 2)
                success |= 0x10;
            if (ro_size == 2)
                success |= 0x20;
        }

        if (idx < 1024)
        {
            if (RWBufs[idx][0] == idx + 1)
                success |= 1;
            if (Bufs[idx][0] == idx + 1)
                success |= 4;
        }
        else
        {
            if (RWBufs[idx][0] == 1)
                success |= 1;
            if (Bufs[idx][0] == 1)
                success |= 4;
        }

        if (idx == 0)
        {
            if (RWBufs[idx][3] == 1)
                success |= 2;
            if (Bufs[idx][3] == 1)
                success |= 8;
        }
        else
        {
            if (RWBufs[idx][3] == 0)
                success |= 2;
            if (Bufs[idx][3] == 0)
                success |= 8;
        }

        RWBufs[idx][0] = success;
    }
#endif
    static const DWORD cs_code_dxbc[] =
    {
        0x43425844, 0xdf9191f9, 0x9533b3a1, 0x029e1c7f, 0x10c8433c, 0x00000001, 0x000006f4, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000006a0, 0x00050051, 0x000001a8, 0x0100086a,
        0x07000059, 0x00308e46, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x07000858,
        0x00307e46, 0x00000000, 0x00000000, 0xffffffff, 0x00004444, 0x00000000, 0x0700089c, 0x0031ee46,
        0x00000000, 0x00000000, 0xffffffff, 0x00004444, 0x00000000, 0x0200005f, 0x00020012, 0x02000068,
        0x00000002, 0x0400009b, 0x00000001, 0x00000001, 0x00000001, 0x0800001e, 0x00100012, 0x00000000,
        0x0002000a, 0x0030800a, 0x00000000, 0x00000000, 0x00000000, 0x0b0000b4, 0x00100012, 0x00000001,
        0x0421e000, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000001, 0x00004001, 0x000000c8,
        0x0b0000b4, 0x00100012, 0x00000001, 0x0421e000, 0x00000000, 0x0010000a, 0x00000000, 0x00004001,
        0x00000002, 0x00004001, 0x00000190, 0x07000079, 0x00100022, 0x00000000, 0x0421ee16, 0x00000000,
        0x0010000a, 0x00000000, 0x07000079, 0x00100042, 0x00000000, 0x04207c96, 0x00000000, 0x0010000a,
        0x00000000, 0x0300001f, 0x0010000a, 0x00000000, 0x07000020, 0x00100082, 0x00000000, 0x0010001a,
        0x00000000, 0x00004001, 0x01000000, 0x0304001f, 0x0010003a, 0x00000000, 0x05000036, 0x00100082,
        0x00000000, 0x00004001, 0x00000010, 0x01000012, 0x05000036, 0x00100082, 0x00000000, 0x00004001,
        0x00000000, 0x01000015, 0x07000020, 0x00100012, 0x00000001, 0x0010002a, 0x00000000, 0x00004001,
        0x01000000, 0x0304001f, 0x0010000a, 0x00000001, 0x0700001e, 0x00100082, 0x00000000, 0x0010003a,
        0x00000000, 0x00004001, 0x00000020, 0x01000015, 0x01000012, 0x07000020, 0x00100022, 0x00000000,
        0x0010001a, 0x00000000, 0x00004001, 0x00000002, 0x0304001f, 0x0010001a, 0x00000000, 0x05000036,
        0x00100082, 0x00000000, 0x00004001, 0x00000010, 0x01000012, 0x05000036, 0x00100082, 0x00000000,
        0x00004001, 0x00000000, 0x01000015, 0x07000020, 0x00100022, 0x00000000, 0x0010002a, 0x00000000,
        0x00004001, 0x00000002, 0x0304001f, 0x0010001a, 0x00000000, 0x0700001e, 0x00100082, 0x00000000,
        0x0010003a, 0x00000000, 0x00004001, 0x00000020, 0x01000015, 0x01000015, 0x0700004f, 0x00100022,
        0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000400, 0x0304001f, 0x0010001a, 0x00000000,
        0x0c0000a3, 0x00100022, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x0421ee16, 0x00000000, 0x0010000a, 0x00000000, 0x0700001e, 0x00100042, 0x00000000, 0x0010000a,
        0x00000000, 0x00004001, 0x00000001, 0x07000020, 0x00100022, 0x00000000, 0x0010002a, 0x00000000,
        0x0010001a, 0x00000000, 0x0304001f, 0x0010001a, 0x00000000, 0x0700001e, 0x00100082, 0x00000000,
        0x0010003a, 0x00000000, 0x00004001, 0x00000001, 0x01000015, 0x0c00002d, 0x00100022, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x04207e16, 0x00000000, 0x0010000a,
        0x00000000, 0x07000020, 0x00100022, 0x00000000, 0x0010002a, 0x00000000, 0x0010001a, 0x00000000,
        0x0304001f, 0x0010001a, 0x00000000, 0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000,
        0x00004001, 0x00000004, 0x01000015, 0x01000012, 0x0c0000a3, 0x00100022, 0x00000000, 0x00004002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0421ee16, 0x00000000, 0x0010000a, 0x00000000,
        0x07000020, 0x00100022, 0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x00000001, 0x0304001f,
        0x0010001a, 0x00000000, 0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001,
        0x00000001, 0x01000015, 0x0c00002d, 0x00100022, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x04207e16, 0x00000000, 0x0010000a, 0x00000000, 0x07000020, 0x00100022,
        0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x00000001, 0x0304001f, 0x0010001a, 0x00000000,
        0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001, 0x00000004, 0x01000015,
        0x01000015, 0x0300001f, 0x0010000a, 0x00000000, 0x0c0000a3, 0x00100022, 0x00000000, 0x00004002,
        0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x0421ee16, 0x00000000, 0x0010000a, 0x00000000,
        0x07000020, 0x00100022, 0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x00000001, 0x0304001f,
        0x0010001a, 0x00000000, 0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001,
        0x00000002, 0x01000015, 0x0c00002d, 0x00100022, 0x00000000, 0x00004002, 0x00000003, 0x00000003,
        0x00000003, 0x00000003, 0x04207e16, 0x00000000, 0x0010000a, 0x00000000, 0x07000020, 0x00100022,
        0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x00000001, 0x0304001f, 0x0010001a, 0x00000000,
        0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001, 0x00000008, 0x01000015,
        0x01000012, 0x0c0000a3, 0x00100022, 0x00000000, 0x00004002, 0x00000003, 0x00000003, 0x00000003,
        0x00000003, 0x0421ee16, 0x00000000, 0x0010000a, 0x00000000, 0x0300001f, 0x0010001a, 0x00000000,
        0x0700001e, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001, 0x00000002, 0x01000015,
        0x0c00002d, 0x00100022, 0x00000000, 0x00004002, 0x00000003, 0x00000003, 0x00000003, 0x00000003,
        0x04207e16, 0x00000000, 0x0010000a, 0x00000000, 0x0300001f, 0x0010001a, 0x00000000, 0x0700001e,
        0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x00004001, 0x00000008, 0x01000015, 0x01000015,
        0x0c0000a4, 0x0421e0f2, 0x00000000, 0x0010000a, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00100ff6, 0x00000000, 0x0100003e,
    };
    static const BYTE cs_code_dxil[] =
    {
        0x44, 0x58, 0x42, 0x43, 0x12, 0x16, 0x7a, 0x18, 0x4c, 0xa3, 0x4f, 0x45, 0x75, 0x3b, 0x7a, 0x31, 0xa3, 0x3a, 0xbc, 0xc4, 0x01, 0x00, 0x00, 0x00, 0x60, 0x09, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x90, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0xe4, 0xad, 0x6f, 0x33, 0x81, 0x0b, 0x71, 0x9b, 0x1d, 0x1b, 0xbe, 0x67, 0x65, 0x4c, 0x5f, 0x44, 0x58, 0x49, 0x4c,
        0x3c, 0x08, 0x00, 0x00, 0x60, 0x00, 0x05, 0x00, 0x0f, 0x02, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x24, 0x08, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde,
        0x21, 0x0c, 0x00, 0x00, 0x06, 0x02, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39,
        0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62, 0x80, 0x18, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xc4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x62, 0x88,
        0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90, 0x11, 0x23, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x31, 0x46, 0x06,
        0x51, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xa8, 0x0d, 0x86, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20, 0x01, 0xd5, 0x06, 0x62,
        0xf8, 0xff, 0xff, 0xff, 0xff, 0x01, 0x90, 0x00, 0x49, 0x18, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x4c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00,
        0x47, 0x00, 0x00, 0x00, 0x32, 0x22, 0x88, 0x09, 0x20, 0x64, 0x85, 0x04, 0x13, 0x23, 0xa4, 0x84, 0x04, 0x13, 0x23, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8c, 0x8c, 0x0b, 0x84, 0xc4, 0x4c,
        0x10, 0x90, 0xc1, 0x08, 0x40, 0x09, 0x00, 0x0a, 0xe6, 0x08, 0xc0, 0xa0, 0x0c, 0xc3, 0x30, 0x10, 0x71, 0xd3, 0x70, 0xf9, 0x13, 0xf6, 0x10, 0x92, 0xbf, 0x12, 0xd2, 0x4a, 0x4c, 0x3e, 0x72, 0xdb,
        0xa8, 0x18, 0x86, 0x61, 0x18, 0xe6, 0x08, 0x10, 0x3a, 0xee, 0x19, 0x2e, 0x7f, 0xc2, 0x1e, 0x42, 0xf2, 0x43, 0xa0, 0x19, 0x16, 0x02, 0x05, 0x48, 0x29, 0x8c, 0x21, 0x19, 0x06, 0x52, 0x0a, 0x32,
        0x0c, 0xc9, 0x30, 0x0c, 0xc3, 0x40, 0x4c, 0x51, 0x80, 0x21, 0x19, 0x86, 0x61, 0x18, 0x86, 0x83, 0x9c, 0x9b, 0x86, 0xcb, 0x9f, 0xb0, 0x87, 0x90, 0xfc, 0x0e, 0x61, 0x88, 0x46, 0x42, 0x9c, 0x46,
        0x42, 0xc4, 0x30, 0x0c, 0x43, 0x21, 0xa0, 0x21, 0x19, 0x28, 0x3a, 0x6a, 0xb8, 0xfc, 0x09, 0x7b, 0x08, 0xc9, 0xe7, 0x36, 0xaa, 0x58, 0x89, 0xc9, 0x47, 0x6e, 0x1b, 0x11, 0xc3, 0x30, 0x0c, 0x85,
        0x98, 0x86, 0x64, 0x20, 0x6a, 0x8e, 0x20, 0x28, 0x46, 0x32, 0x1c, 0xc3, 0x60, 0xd1, 0x35, 0x10, 0x30, 0x93, 0x36, 0x0e, 0xec, 0x10, 0x0e, 0xf3, 0x30, 0x0f, 0x6e, 0x20, 0x0a, 0xf5, 0x60, 0x0e,
        0xe6, 0x50, 0x0e, 0xf2, 0xc0, 0x07, 0xf5, 0xe0, 0x0e, 0xf3, 0x90, 0x0e, 0xe7, 0xe0, 0x0e, 0xe5, 0x40, 0x0e, 0x60, 0x90, 0x0e, 0xee, 0x40, 0x0f, 0x7e, 0x80, 0x82, 0x21, 0x01, 0x34, 0xda, 0x66,
        0xe2, 0xc6, 0x81, 0x1d, 0xc2, 0x61, 0x1e, 0xe6, 0xc1, 0x0d, 0x64, 0xe1, 0x16, 0x44, 0xa1, 0x1e, 0xcc, 0xc1, 0x1c, 0xca, 0x41, 0x1e, 0xf8, 0xa0, 0x1e, 0xdc, 0x61, 0x1e, 0xd2, 0xe1, 0x1c, 0xdc,
        0xa1, 0x1c, 0xc8, 0x01, 0x0c, 0xd2, 0xc1, 0x1d, 0xe8, 0xc1, 0x0f, 0x50, 0x30, 0x24, 0x40, 0x47, 0xde, 0x19, 0x29, 0x10, 0x01, 0x8c, 0x84, 0x82, 0x81, 0xc0, 0x39, 0x02, 0x50, 0x98, 0x02, 0x00,
        0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30,
        0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a,
        0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07,
        0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60,
        0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x14, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x30, 0x40, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4,
        0x71, 0x80, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xc8, 0x13, 0x01, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x90, 0x87, 0x02, 0x02, 0x40, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0xcf, 0x05, 0x04, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x20, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14,
        0x19, 0x11, 0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x4a, 0x60, 0x04, 0xa0, 0x18, 0x6a, 0xa0, 0x28, 0x4a, 0xa1, 0x10, 0xc8, 0x1a, 0x01, 0x20, 0xb1, 0x00, 0x01, 0x09, 0xe8,
        0x9b, 0x01, 0xa0, 0x70, 0x06, 0x80, 0xb8, 0x19, 0x00, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0x44, 0x35, 0x18, 0x63, 0x0b,
        0x73, 0x3b, 0x03, 0xb1, 0x2b, 0x93, 0x9b, 0x4b, 0x7b, 0x73, 0x03, 0x99, 0x71, 0xb9, 0x01, 0x41, 0xa1, 0x0b, 0x3b, 0x9b, 0x7b, 0x91, 0x2a, 0x62, 0x2a, 0x0a, 0x9a, 0x2a, 0xfa, 0x9a, 0xb9, 0x81,
        0x79, 0x31, 0x4b, 0x73, 0x0b, 0x63, 0x4b, 0xd9, 0x10, 0x04, 0x13, 0x84, 0x01, 0x99, 0x20, 0x0c, 0xc9, 0x06, 0x61, 0x20, 0x26, 0x08, 0x83, 0xb2, 0x41, 0x18, 0x0c, 0x0a, 0x63, 0x73, 0x1b, 0x06,
        0xc4, 0x20, 0x26, 0x08, 0xdc, 0x44, 0x60, 0x82, 0x30, 0x2c, 0x13, 0x84, 0x81, 0x99, 0x20, 0x0c, 0xcd, 0x06, 0x81, 0x70, 0x36, 0x24, 0x84, 0xb2, 0x10, 0x04, 0xd3, 0x10, 0xcf, 0x86, 0x00, 0x9a,
        0x20, 0x7c, 0xd1, 0x04, 0xc1, 0x7a, 0x36, 0x2c, 0x84, 0xb4, 0x10, 0x04, 0xd3, 0x4c, 0xd3, 0xf4, 0x6c, 0x08, 0xa8, 0x09, 0x42, 0x18, 0x48, 0x13, 0x84, 0xc1, 0xd9, 0x80, 0x10, 0xd6, 0x42, 0x10,
        0xc3, 0x05, 0x6c, 0x08, 0xb0, 0x0d, 0x44, 0x54, 0x65, 0xc0, 0x04, 0x41, 0x00, 0x48, 0xb4, 0x85, 0xa5, 0xb9, 0x4d, 0x10, 0xc4, 0x00, 0xda, 0x30, 0x0c, 0xc3, 0xb0, 0x81, 0x20, 0xba, 0xcb, 0xdb,
        0x50, 0x6c, 0x1c, 0xa0, 0x7d, 0x55, 0xd8, 0xd8, 0xec, 0xda, 0x5c, 0xd2, 0xc8, 0xca, 0xdc, 0xe8, 0xa6, 0x04, 0x41, 0x15, 0x32, 0x3c, 0x17, 0xbb, 0x32, 0xb9, 0xb9, 0xb4, 0x37, 0xb7, 0x29, 0x01,
        0xd1, 0x84, 0x0c, 0xcf, 0xc5, 0x2e, 0x8c, 0xcd, 0xae, 0x4c, 0x6e, 0x4a, 0x60, 0xd4, 0x21, 0xc3, 0x73, 0x99, 0x43, 0x0b, 0x23, 0x2b, 0x93, 0x6b, 0x7a, 0x23, 0x2b, 0x63, 0x9b, 0x12, 0x20, 0x65,
        0xc8, 0xf0, 0x5c, 0xe4, 0xca, 0xe6, 0xde, 0xea, 0xe4, 0xc6, 0xca, 0xe6, 0xa6, 0x04, 0x59, 0x1d, 0x32, 0x3c, 0x97, 0x32, 0x37, 0x3a, 0xb9, 0x3c, 0xa8, 0xb7, 0x34, 0x37, 0xba, 0xb9, 0x29, 0xc1,
        0x07, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07,
        0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43,
        0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76,
        0x78, 0x87, 0x70, 0x20, 0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8,
        0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68,
        0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71,
        0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4,
        0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43,
        0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19,
        0xd1, 0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20, 0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x30, 0x83, 0x81, 0xc8, 0x01, 0x1f, 0xdc,
        0x40, 0x1c, 0xe4, 0xa1, 0x1c, 0xc2, 0x61, 0x1d, 0xdc, 0x40, 0x1c, 0xe4, 0x01, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x36, 0x50, 0x0d, 0x97, 0xef, 0x3c, 0x3e, 0x30,
        0x39, 0x0c, 0x22, 0x6c, 0x48, 0x83, 0x3e, 0x3e, 0x72, 0xdb, 0x26, 0x40, 0x0d, 0x97, 0xef, 0x3c, 0x7e, 0x40, 0x15, 0x05, 0x11, 0x95, 0x0e, 0x30, 0xf8, 0xc8, 0x6d, 0x1b, 0x41, 0x35, 0x5c, 0xbe,
        0xf3, 0xf8, 0x01, 0x55, 0x14, 0x44, 0xc4, 0x4e, 0x4e, 0x44, 0xf8, 0xc8, 0x6d, 0x9b, 0xc1, 0x36, 0x5c, 0xbe, 0xf3, 0xf8, 0x42, 0x40, 0x15, 0x05, 0x11, 0x95, 0x0e, 0x30, 0x94, 0x84, 0x01, 0x08,
        0x98, 0x8f, 0xdc, 0xb6, 0x1d, 0x48, 0xc3, 0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04, 0x30, 0x11, 0x21, 0xd0, 0x0c, 0x0b, 0x61, 0x05, 0xd3, 0x70, 0xf9, 0xce, 0xe3, 0x1b, 0xc4, 0xd4, 0x21, 0x0c, 0xd1,
        0x48, 0x88, 0xd3, 0x48, 0x16, 0x20, 0x0d, 0x97, 0xef, 0x3c, 0xfe, 0x74, 0x44, 0x04, 0x30, 0x88, 0x83, 0x8f, 0xdc, 0xb6, 0x01, 0x10, 0x0c, 0x80, 0x34, 0x00, 0x00, 0x00, 0x61, 0x20, 0x00, 0x00,
        0x8e, 0x00, 0x00, 0x00, 0x13, 0x04, 0x4a, 0x2c, 0x10, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x34, 0xcc, 0x00, 0x14, 0x62, 0x40, 0x11, 0x94, 0x41, 0xc9, 0x15, 0x67, 0x40, 0x41, 0x06, 0x14,
        0x20, 0x20, 0x20, 0x44, 0x81, 0x14, 0x50, 0x41, 0x94, 0x6e, 0x40, 0xd9, 0x15, 0xe4, 0x40, 0x81, 0x1a, 0x14, 0x20, 0x44, 0x29, 0x06, 0xd0, 0x51, 0x04, 0x25, 0x30, 0x02, 0x50, 0x1e, 0x00, 0x00,
        0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0x70, 0x89, 0x41, 0x44, 0x80, 0x01, 0x18, 0x68, 0x23, 0x06, 0x06, 0x00, 0x82, 0x60, 0x40, 0xa4, 0xc1, 0x12, 0x06, 0x23, 0x06, 0x07, 0x00, 0x82, 0x60, 0x40,
        0x95, 0xc1, 0x22, 0x88, 0xc1, 0x68, 0x42, 0x00, 0x54, 0x30, 0x40, 0x05, 0x65, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0x70, 0xa1, 0xc1, 0x95, 0x98, 0x41, 0x00, 0x06, 0x23, 0x06, 0x0b, 0x00,
        0x82, 0x60, 0xc0, 0xb4, 0xc1, 0x15, 0x9c, 0x01, 0x1a, 0x70, 0xdc, 0x33, 0x62, 0xb0, 0x00, 0x20, 0x08, 0x06, 0x8c, 0x1b, 0x60, 0x02, 0x1a, 0x6c, 0x5d, 0xf7, 0x8c, 0x18, 0x1c, 0x00, 0x08, 0x82,
        0x41, 0xd4, 0x06, 0xd8, 0xe0, 0x8d, 0x26, 0x04, 0x40, 0x1d, 0x6b, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0x70, 0xb9, 0x41, 0xe7, 0xb0, 0x41, 0x60, 0x06, 0x23, 0x06, 0x07, 0x00, 0x82, 0x60,
        0x10, 0xc5, 0x01, 0x17, 0x88, 0xc1, 0x68, 0x42, 0x00, 0x0c, 0x37, 0x2c, 0x6f, 0x00, 0x06, 0xb3, 0x0c, 0x81, 0x10, 0x0c, 0x37, 0x18, 0x1e, 0x18, 0x4c, 0x37, 0x78, 0x71, 0x10, 0x0c, 0x37, 0x10,
        0x60, 0x00, 0x06, 0x25, 0x7c, 0x3b, 0xdd, 0x10, 0x0c, 0xc2, 0x2c, 0xc1, 0x30, 0xdc, 0xb0, 0xa0, 0x01, 0x18, 0x4c, 0x37, 0x8c, 0x81, 0x1d, 0x04, 0xc3, 0x0d, 0x89, 0x1a, 0x80, 0x41, 0x09, 0x64,
        0xb0, 0xd3, 0x0d, 0xc1, 0x20, 0xcc, 0x12, 0x0c, 0x03, 0x15, 0x03, 0x13, 0x08, 0xc2, 0x70, 0xc3, 0x15, 0x06, 0x64, 0x30, 0xcb, 0x40, 0x14, 0xc1, 0x88, 0x01, 0x02, 0x80, 0x20, 0x18, 0x28, 0xa3,
        0x00, 0x07, 0x16, 0x1f, 0xc4, 0xc1, 0x68, 0x42, 0x00, 0x94, 0xf6, 0x07, 0x30, 0xdc, 0x20, 0x04, 0x60, 0x70, 0xc1, 0x10, 0x15, 0x1c, 0x3b, 0x62, 0x80, 0x00, 0x20, 0x08, 0x06, 0x4a, 0x2a, 0xd8,
        0x81, 0x25, 0x0a, 0x77, 0x30, 0x9a, 0x10, 0x00, 0xc3, 0x0d, 0x81, 0x01, 0x06, 0x45, 0x80, 0xc2, 0x4e, 0x37, 0x04, 0x85, 0x30, 0x4b, 0x60, 0x8c, 0x18, 0x20, 0x00, 0x08, 0x82, 0x81, 0xe2, 0x0a,
        0x7b, 0x10, 0x06, 0xa7, 0xc0, 0x07, 0xa3, 0x09, 0x01, 0x30, 0xdc, 0x10, 0xa8, 0x02, 0x18, 0x5c, 0x30, 0x44, 0x05, 0xd1, 0x8e, 0x18, 0x20, 0x00, 0x08, 0x82, 0x81, 0x32, 0x0b, 0xa0, 0x00, 0x06,
        0xac, 0x10, 0x0a, 0xa3, 0x09, 0x01, 0x30, 0xdc, 0x10, 0xbc, 0x02, 0x18, 0x14, 0xa1, 0x0a, 0x3b, 0xdd, 0x10, 0x14, 0xc2, 0x2c, 0x81, 0x31, 0x50, 0x31, 0x58, 0x84, 0x50, 0x54, 0x50, 0x0a, 0x3b,
        0xcb, 0x70, 0x20, 0x64, 0x30, 0x62, 0x80, 0x00, 0x20, 0x08, 0x06, 0x8a, 0x2e, 0x9c, 0x42, 0x1b, 0x94, 0x02, 0x2a, 0x8c, 0x26, 0x04, 0xc0, 0x70, 0x43, 0x60, 0x0b, 0x60, 0x30, 0xdd, 0x40, 0x14,
        0xc1, 0x88, 0x01, 0x02, 0x80, 0x20, 0x18, 0x28, 0xbe, 0xb0, 0x0a, 0x6b, 0x90, 0x0a, 0xac, 0x30, 0x9a, 0x10, 0x00, 0xc3, 0x0d, 0x81, 0x2e, 0x80, 0x41, 0x11, 0xa5, 0xb0, 0xd3, 0x0d, 0x41, 0x21,
        0xcc, 0x12, 0x24, 0x23, 0x06, 0x08, 0x00, 0x82, 0x60, 0xa0, 0x8c, 0x03, 0x2c, 0xd8, 0x81, 0x2b, 0xc4, 0xc2, 0x68, 0x42, 0x00, 0x0c, 0x37, 0x04, 0xbe, 0x00, 0x06, 0xd3, 0x0d, 0x8d, 0x13, 0x8c,
        0x18, 0x20, 0x00, 0x08, 0x82, 0x81, 0x72, 0x0e, 0xb4, 0x40, 0x07, 0xb2, 0x50, 0x0b, 0xa3, 0x09, 0x01, 0x30, 0xdc, 0x10, 0x88, 0x03, 0x18, 0x14, 0xe1, 0x0a, 0x3b, 0xdd, 0x10, 0x14, 0xc2, 0x2c,
        0x41, 0x32, 0x50, 0x31, 0x50, 0x87, 0x80, 0x8c, 0x18, 0x34, 0x00, 0x08, 0x82, 0x81, 0xb3, 0x0e, 0xab, 0x00, 0x0a, 0xe6, 0xb0, 0x0b, 0x41, 0x10, 0x04, 0xa7, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    if (!init_compute_test_context(&context))
        return;

    if (use_dxil && !context_supports_dxil(&context))
    {
        destroy_test_context(&context);
        return;
    }

    command_list = context.list;
    queue = context.queue;

    root_signature_desc.NumParameters = 2;
    root_signature_desc.Flags = 0;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = NULL;
    root_signature_desc.pParameters = root_parameters;

    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_parameters[0].DescriptorTable.NumDescriptorRanges = 2;
    root_parameters[0].DescriptorTable.pDescriptorRanges = descriptor_ranges;

    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_parameters[1].Constants.ShaderRegister = 0;
    root_parameters[1].Constants.RegisterSpace = 0;
    root_parameters[1].Constants.Num32BitValues = 1;

    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].BaseShaderRegister = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;
    descriptor_ranges[0].NumDescriptors = UINT_MAX;
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    descriptor_ranges[1].RegisterSpace = 0;
    descriptor_ranges[1].BaseShaderRegister = 0;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = 500000;
    descriptor_ranges[1].NumDescriptors = UINT_MAX;
    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    hr = create_root_signature(context.device, &root_signature_desc, &context.root_signature);
    ok(SUCCEEDED(hr), "Failed to create root signature, hr %#x.\n", hr);

    output_buffer = create_default_buffer(context.device, 64 * 1024 * 1024, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    input_buffer = create_default_buffer(context.device, 64 * 1024 * 1024, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

    context.pipeline_state = create_compute_pipeline_state(context.device,
        context.root_signature,
        shader_bytecode(use_dxil ? (const void *)cs_code_dxil : (const void *)cs_code_dxbc,
            use_dxil ? sizeof(cs_code_dxil) : sizeof(cs_code_dxbc)));

    cpu_heap = create_cpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000);
    gpu_heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000);
    host_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(cpu_heap);
    visible_handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(gpu_heap);
    descriptor_size = ID3D12Device_GetDescriptorHandleIncrementSize(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    /* This will likely completely exhaust NV drivers memory unless typed offset buffer is implemented properly. */
    for (j = 10; j >= 2; j--)
    {
        for (i = 0; i < 1000000; i++)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE h = host_handle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC view;
            view.Format = DXGI_FORMAT_R32_UINT;
            view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
            view.Buffer.StructureByteStride = 0;
            view.Buffer.CounterOffsetInBytes = 0;
            view.Buffer.FirstElement = 4 * i;
            /* Final iteration is j == 2. First descriptor covers entire view so we can clear the buffer to something non-zero in the loop below. */
            view.Buffer.NumElements = i == 0 ? (16 * 1024 * 1024) : j;
            h.ptr += i * descriptor_size;
            ID3D12Device_CreateUnorderedAccessView(context.device, output_buffer, NULL, &view, h);
        }

        for (i = 500000; i < 1000000; i++)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE h = host_handle;
            D3D12_SHADER_RESOURCE_VIEW_DESC view;
            view.Format = DXGI_FORMAT_R32_UINT;
            view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            view.Buffer.StructureByteStride = 0;
            view.Buffer.FirstElement = 4 * (i - 500000);
            view.Buffer.NumElements = i == 500000 ? (16 * 1024 * 1024) : j;
            view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            h.ptr += i * descriptor_size;
            ID3D12Device_CreateShaderResourceView(context.device, input_buffer, &view, h);
        }
    }

    {
        D3D12_CPU_DESCRIPTOR_HANDLE h = host_handle;
        h.ptr += descriptor_size;
        ID3D12Device_CopyDescriptorsSimple(context.device, 16, visible_handle, h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        /* Tests that we handle the case where only the offset changes. */
        ID3D12Device_CopyDescriptorsSimple(context.device, 1000000, visible_handle, host_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &gpu_heap);

    /* Check that UAV clear works with typed offsets. */
    for (i = 0; i < 1024; i++)
    {
        const UINT clear_value[4] = { i + 1, i + 2, i + 3, i + 4 };
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc;
        D3D12_RESOURCE_BARRIER barrier;

        cpu_desc = host_handle;
        gpu_desc = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(gpu_heap);
        cpu_desc.ptr += i * descriptor_size;
        gpu_desc.ptr += i * descriptor_size;

        ID3D12GraphicsCommandList_ClearUnorderedAccessViewUint(command_list, gpu_desc, cpu_desc,
            output_buffer, clear_value, 0, NULL);

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = output_buffer;
        ID3D12GraphicsCommandList_ResourceBarrier(command_list, 1, &barrier);
    }

    transition_resource_state(command_list, output_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    ID3D12GraphicsCommandList_CopyBufferRegion(command_list, input_buffer, 0, output_buffer, 0, 64 * 1024 * 1024);
    transition_resource_state(command_list, output_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transition_resource_state(command_list, input_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &gpu_heap);
    ID3D12GraphicsCommandList_SetComputeRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(command_list, 0, ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(gpu_heap));

    for (i = 0; i < 500000; i += 50000)
    {
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(command_list, 1, i, 0);
        ID3D12GraphicsCommandList_Dispatch(command_list, 50000, 1, 1);
    }

    transition_resource_state(command_list, output_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(output_buffer, DXGI_FORMAT_R32_UINT, &rb, queue, command_list);

    /* Don't bother testing all 1M descriptors, after 64k we should be pretty sure that we got it right. */
    for (i = 0; i < 64 * 1024; i++)
    {
        UINT value, reference;
        value = get_readback_uint(&rb, 4 * i, 0, 0);
        reference = 0x3f;
        ok(value == reference, "Readback value for iteration #%u, elem %u is: %u (ref: %u)\n", i, 0, value, reference);

        value = get_readback_uint(&rb, 4 * i + 1, 0, 0);
        reference = i < 1024 ? (201 + i) : 201;
        ok(value == reference, "Readback value for iteration #%u, elem %u is: %u (ref: %u)\n", i, 1, value, reference);

        for (j = 2; j < 4; j++)
        {
            value = get_readback_uint(&rb, 4 * i + j, 0, 0);
            reference = j == 2 && i == 0 ? 401 : 1;
            ok(value == reference, "Readback value for iteration #%u, elem %u is: %u (ref: %u)\n", i, j, value, reference);
        }
    }

    release_resource_readback(&rb);
    reset_command_list(command_list, context.allocator);

    ID3D12Resource_Release(output_buffer);
    ID3D12Resource_Release(input_buffer);
    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12DescriptorHeap_Release(gpu_heap);
    destroy_test_context(&context);
}

void test_typed_buffers_many_objects_dxbc(void)
{
    test_typed_buffers_many_objects(false);
}

void test_typed_buffers_many_objects_dxil(void)
{
    test_typed_buffers_many_objects(true);
}

void test_view_min_lod(void)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC view_desc;
    ID3D12GraphicsCommandList *command_list;
    const D3D12_SHADER_BYTECODE *ps = NULL;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    ID3D12PipelineState *pso = NULL;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12CommandQueue *queue;
    ID3D12Resource *texture;
    unsigned int offset;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_view_min_lod_load_code[] =
    {
#if 0
        Texture2D tex;
        float testLod;

        float4 main() : SV_Target
        {
            return tex.Load(int3(0, 0, int(testLod)));
        }
#endif
        0x43425844, 0xe23be9df, 0xf78327b8, 0xb2d9d572, 0xefa569ae, 0x00000001, 0x00000118, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x000000a0, 0x00000050, 0x00000028,
        0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0600001b, 0x001000c2,
        0x00000000, 0x00208006, 0x00000000, 0x00000000, 0x08000036, 0x00100032, 0x00000000, 0x00004002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8900002d, 0x800000c2, 0x00155543, 0x001020f2,
        0x00000000, 0x00100e46, 0x00000000, 0x00107e46, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_view_min_lod_load = {ps_view_min_lod_load_code, sizeof(ps_view_min_lod_load_code)};
    static const DWORD ps_view_min_lod_sample_code[] =
    {
#if 0
        Texture2D tex;
        SamplerState s;
        float testLod;

        float4 main() : SV_Target
        {
            return tex.SampleLevel(s, float2(0, 0), testLod);
        }
#endif
        0x43425844, 0x6447f634, 0xc09020fb, 0xdffd3b83, 0xabf31dab, 0x00000001, 0x00000104, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x0000008c, 0x00000050, 0x00000023,
        0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x03000065, 0x001020f2, 0x00000000, 0x91000048,
        0x800000c2, 0x00155543, 0x001020f2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0020800a, 0x00000000, 0x00000000,
        0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps_view_min_lod_sample = {ps_view_min_lod_sample_code, sizeof(ps_view_min_lod_sample_code)};
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    unsigned int texture_data[8 * 8 + 4 * 4 + 2 * 2 + 1 * 1];
    D3D12_SUBRESOURCE_DATA resource_data[4];
    static const struct
    {
        const D3D12_SHADER_BYTECODE *ps;
        int most_detailed_mip;
        float test_lod;
        float min_lod;
        unsigned int expected_color;
        bool is_todo;
    }
    tests[] =
    {
        {&ps_view_min_lod_load, 0, -1.0f, 0.0f, 0x00000000},
        {&ps_view_min_lod_load, 0,  0.0f, 0.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_load, 0,  1.0f, 0.0f, 0xffffffff},
        {&ps_view_min_lod_load, 0,  2.0f, 0.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_load, 0,  3.0f, 0.0f, 0xffffffff},

        {&ps_view_min_lod_load, 0, -1.0f, 1.0f, 0x00000000},
        {&ps_view_min_lod_load, 0,  0.0f, 1.0f, 0x00000000},
        {&ps_view_min_lod_load, 0,  1.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_load, 0,  2.0f, 1.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_load, 0,  3.0f, 1.0f, 0xffffffff},

        {&ps_view_min_lod_load, 1, -1.0f, 1.0f, 0x00000000},
        {&ps_view_min_lod_load, 1,  0.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_load, 1,  1.0f, 1.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_load, 1,  2.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_load, 1,  3.0f, 1.0f, 0x00000000},

        {&ps_view_min_lod_load, 1, -1.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_load, 1,  0.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_load, 1,  1.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_load, 1,  2.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_load, 1,  3.0f, 9.0f, 0x00000000},

        /* floor(minLOD) behavior for integer fetch. */
        {&ps_view_min_lod_load, 1, 0.0f, 1.9f, 0xffffffff},
        {&ps_view_min_lod_load, 1, 0.0f, 2.0f, 0x00000000},
        {&ps_view_min_lod_load, 1, 1.0f, 2.9f, 0x0f0f0f0f},
        {&ps_view_min_lod_load, 1, 1.0f, 3.0f, 0x00000000},

        {&ps_view_min_lod_sample, 0, -1.0f, 0.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 0,  0.0f, 0.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 0,  1.0f, 0.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 0,  2.0f, 0.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 0,  3.0f, 0.0f, 0xffffffff},

        {&ps_view_min_lod_sample, 0, -1.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 0,  0.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 0,  1.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 0,  2.0f, 1.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 0,  3.0f, 1.0f, 0xffffffff},

        {&ps_view_min_lod_sample, 1, -1.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 1,  0.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 1,  1.0f, 1.0f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 1,  2.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 1,  3.0f, 1.0f, 0xffffffff},
        {&ps_view_min_lod_sample, 1,  4.0f, 1.0f, 0xffffffff},

        {&ps_view_min_lod_sample, 1, -1.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_sample, 1,  0.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_sample, 1,  1.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_sample, 1,  2.0f, 9.0f, 0x00000000},
        {&ps_view_min_lod_sample, 1,  3.0f, 9.0f, 0x00000000},

        /* Tests rounding behavior for POINT mip filter. Nearest mip level is selected after clamp on AMD and NV,
         * but not Intel, oddly enough. */
        {&ps_view_min_lod_sample, 1, 0.25f, 1.00f, 0xffffffff},
        {&ps_view_min_lod_sample, 1, 0.25f, 1.25f, 0xffffffff},
        {&ps_view_min_lod_sample, 1, 0.25f, 2.00f, 0x0f0f0f0f},
        {&ps_view_min_lod_sample, 1, -0.25f, 1.00f, 0xffffffff},
        {&ps_view_min_lod_sample, 1, -0.25f, 1.25f, 0xffffffff},
        {&ps_view_min_lod_sample, 1, -0.25f, 2.00f, 0x0f0f0f0f},
        /* Intel HW fails these tests on native, D3D11 functional spec is not well defined here. */
        /*{&ps_view_min_lod_sample, 1, 0.25f, 1.75f, 0x0f0f0f0f},*/
        /*{&ps_view_min_lod_sample, 1, -0.25f, 1.75f, 0x0f0f0f0f},*/
    };

    /* Alternate mip colors */
    offset = 0;
    for (i = 0; i < 4; i++)
    {
        const unsigned int size = 8u >> i;

        resource_data[i] = (D3D12_SUBRESOURCE_DATA) {&texture_data[offset], sizeof(unsigned int) * size};
        memset(&texture_data[offset], (i % 2 == 0) ? 0x0F : 0xFF, sizeof(unsigned int) * size * size);
        offset += size * size;
    }

    memset(&desc, 0, sizeof(desc));
    desc.no_root_signature = true;
    if (!init_test_context(&context, &desc))
        return;
    command_list = context.list;
    queue = context.queue;

    context.root_signature = create_texture_root_signature(context.device,
            D3D12_SHADER_VISIBILITY_PIXEL, 4, 0);

    init_pipeline_state_desc(&pso_desc, context.root_signature,
            context.render_target_desc.Format, NULL, NULL, NULL);

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    gpu_handle = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap);

    texture = create_default_texture2d(context.device,
            8, 8, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_RESOURCE_STATE_COPY_DEST);
    upload_texture_data(texture, resource_data, 4, queue, command_list);
    reset_command_list(command_list, context.allocator);
    transition_resource_state(command_list, texture,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        vkd3d_test_set_context("Test %u", i);
        
        if (ps != tests[i].ps)
        {
            if (pso)
                ID3D12PipelineState_Release(pso);

            ps = tests[i].ps;
            pso_desc.PS = *tests[i].ps;
            hr = ID3D12Device_CreateGraphicsPipelineState(context.device, &pso_desc,
                    &IID_ID3D12PipelineState, (void **)&pso);
            ok(hr == S_OK, "Failed to create graphics pipeline state, hr %#x.\n", hr);
        }

        view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.Texture2D.MostDetailedMip = tests[i].most_detailed_mip;
        view_desc.Texture2D.MipLevels = -1;
        view_desc.Texture2D.PlaneSlice = 0;
        view_desc.Texture2D.ResourceMinLODClamp = tests[i].min_lod;

        ID3D12Device_CreateShaderResourceView(context.device, texture, &view_desc,
                ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv, red, 0, NULL);

        ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv, false, NULL);
        ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
        ID3D12GraphicsCommandList_SetPipelineState(command_list, pso);
        ID3D12GraphicsCommandList_SetDescriptorHeaps(command_list, 1, &heap);
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(command_list, 0, gpu_handle);
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
        ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 1, 1, &tests[i].test_lod, 0);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

        transition_resource_state(command_list, context.render_target,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        todo_if(tests[i].is_todo) check_sub_resource_uint(context.render_target, 0, queue, command_list, tests[i].expected_color, 0);

        reset_command_list(command_list, context.allocator);
        transition_resource_state(command_list, context.render_target,
                D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    vkd3d_test_set_context(NULL);

    ID3D12PipelineState_Release(pso);
    ID3D12Resource_Release(texture);
    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_typed_srv_uav_cast(void)
{
    /* Test rules for CastFullyTypedFormat.
     * This is more of a meta-test. It's not supposed to generate any real results, but we can observe outputs
     * from D3D12 validation layers and VK validation layers to sanity check our assumptions.
     * Should also serve as more clear documentation on actual behavior. */
    D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv;
    struct test_context context;
    ID3D12Resource *uav_texture;
    ID3D12DescriptorHeap *heap;
    ID3D12Resource *texture;
    VKD3D_UNUSED HRESULT hr;
    unsigned int i;

    struct test
    {
        DXGI_FORMAT image;
        DXGI_FORMAT view;
        bool valid_srv;
        bool valid_uav;
    };

    /* Rules:
     * FLOAT cannot be cast to non-FLOAT and vice versa.
     * UNORM cannot be cast to SNORM and vice versa.
     * Everything else is fair game as long as it's within same family.
     * UAVs have some weird legacy rules which were probably inherited from D3D11 ... */
    static const struct test tests[] =
    {
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, true, true },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true, false },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SNORM, false, false },

        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM, false, false },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, false, false },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM, true, true },

        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UNORM, true, true },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true, false },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SNORM, true, true },

        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, true, true },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true, false },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UINT, true, true },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SNORM, true, true },

        /* FLOAT cannot cast with UINT or SINT. */
        { DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_UINT, false, false },
        { DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_SINT, false, false },
        { DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_UINT, false, false },
        { DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_SINT, false, false },
        { DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16_FLOAT, false, false },
        { DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_FLOAT, false, false },

        /* 5.3.9.5 from D3D11 functional spec. 32-bit typeless formats
         * can be viewed as R32{U,I,F}. The D3D12 validation runtime appears to be buggy
         * and also allows fully typed views even if bits per component don't match.
         * This feature is derived from legacy D3D11 jank, so assume the validation layers are
         * just buggy. */

        { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R32_FLOAT, false, true },
        { DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R32_FLOAT, false, true },

        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R32_FLOAT, false, false },
        { DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R32_UINT, false, false },
        { DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R32_SINT, false, false },

        /* D3D12 validation does not complain about these, but it should according to D3D11 functional spec.
         * No docs for D3D12 say otherwise.
         * These tests can trip assertions in drivers since we will not emit MUTABLE at all
         * for some of these tests. */
#if 0
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R32_UINT, false, true },
        { DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R32_SINT, false, true },
        { DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R32_FLOAT, false, true },
#endif
    };

    if (!init_compute_test_context(&context))
        return;

    if (FAILED(ID3D12Device_CheckFeatureSupport(context.device, D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3))) ||
            !options3.CastingFullyTypedFormatSupported)
    {
        skip("CastingFullyTypedFormat is not supported, skipping ...\n");
        destroy_test_context(&context);
        return;
    }

    memset(&srv, 0, sizeof(srv));
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;

    memset(&uav, 0, sizeof(uav));
    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

#define PURGE() do { \
    if (uav_texture) ID3D12Resource_Release(uav_texture); \
    if (texture) ID3D12Resource_Release(texture); \
    if (heap) ID3D12DescriptorHeap_Release(heap); \
    uav_texture = NULL; \
    texture = NULL; \
    heap = NULL;     \
} while(0)

#define PURGE_CONTEXT() do { \
    PURGE(); \
    destroy_test_context(&context); \
    context.device = NULL; \
} while(0)

#define BEGIN_CONTEXT() do { \
    if (!context.device) \
        init_compute_test_context(&context); \
} while(0)

#define BEGIN() do { \
    BEGIN_CONTEXT(); \
    uav_texture = create_default_texture2d(context.device, 1024, 1024, 1, 1, tests[i].image, \
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST); \
    texture = create_default_texture2d(context.device, 1024, 1024, 1, 1, tests[i].image, \
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST); \
    heap = create_cpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1); \
} while(0)

    /* When enabled, only intended to pass on real runtime.
     * The native runtime actually performs validation and will trip device lost if a mistake is made.
     * We don't validate this in vkd3d-proton. */
#define VERIFY_FAILURE_CASES 0

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        vkd3d_test_set_context("Test %u", i);

        BEGIN();

        srv.Format = tests[i].view;
        uav.Format = tests[i].view;

        if (tests[i].valid_srv || VERIFY_FAILURE_CASES)
        {
            ID3D12Device_CreateShaderResourceView(context.device, texture, &srv,
                    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

            if (!tests[i].valid_srv && VERIFY_FAILURE_CASES)
            {
                hr = ID3D12Device_GetDeviceRemovedReason(context.device);

                if ((tests[i].image == DXGI_FORMAT_R8G8B8A8_UNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_SNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))
                {
                    /* It is a documented bug that the runtime forgot to validate UNORM <-> SNORM cast, so trip errors here. */
                    hr = E_FAIL;
                }

                ok(FAILED(hr), "Unexpected hr %#x\n", hr);
                PURGE_CONTEXT();
                BEGIN();
            }

            ID3D12Device_CreateShaderResourceView(context.device, uav_texture, &srv,
                    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

            if (!tests[i].valid_srv && VERIFY_FAILURE_CASES)
            {
                hr = ID3D12Device_GetDeviceRemovedReason(context.device);

                if ((tests[i].image == DXGI_FORMAT_R8G8B8A8_UNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_SNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))
                {
                    /* It is a documented bug that the runtime forgot to validate UNORM <-> SNORM cast, so trip errors here. */
                    hr = E_FAIL;
                }

                ok(FAILED(hr), "Unexpected hr %#x\n", hr);
                PURGE_CONTEXT();
                BEGIN();
            }
        }

        if (tests[i].valid_uav || VERIFY_FAILURE_CASES)
        {
            ID3D12Device_CreateUnorderedAccessView(context.device, uav_texture, NULL, &uav,
                    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

            if (!tests[i].valid_uav && VERIFY_FAILURE_CASES)
            {
                hr = ID3D12Device_GetDeviceRemovedReason(context.device);

                if ((tests[i].image == DXGI_FORMAT_R8G8B8A8_UNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_SNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM) ||
                    (tests[i].image == DXGI_FORMAT_R8G8B8A8_SNORM && tests[i].view == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))
                {
                    /* It is a documented bug that the runtime forgot to validate UNORM <-> SNORM cast, so trip errors here. */
                    hr = E_FAIL;
                }

                ok(FAILED(hr), "Unexpected hr %#x\n", hr);
                PURGE_CONTEXT();
                BEGIN();
            }
        }

        hr = ID3D12Device_GetDeviceRemovedReason(context.device);
        ok(SUCCEEDED(hr), "Unexpected hr %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            PURGE();
        }
        else
        {
            PURGE_CONTEXT();
            BEGIN_CONTEXT();
        }
    }

    vkd3d_test_set_context(NULL);

    destroy_test_context(&context);
}

void test_typed_srv_cast_clear(void)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3;
    D3D12_HEAP_PROPERTIES heap_properties;
    struct test_context_desc test_desc;
    D3D12_RENDER_TARGET_VIEW_DESC rtv;
    D3D12_CLEAR_VALUE clear_value;
    struct test_context context;
    unsigned int test_iteration;
    ID3D12DescriptorHeap *heap;
    D3D12_RESOURCE_DESC desc;
    ID3D12Resource *texture;
    unsigned int i, j;
    FLOAT colors[4];
    bool fast_clear;

    struct test
    {
        DXGI_FORMAT image;
        DXGI_FORMAT view;
        float clear_value;
        float optimized_clear_value;
        uint32_t expected_component;
        uint32_t ulp;
    };

    /* RADV currently misses some cases where fast clear triggers for signed <-> unsigned and we get weird results. */

    static const struct test tests[] =
    {
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SINT, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SINT, 1.0f, 1.0f / 255.0f, 1 },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SINT, 127.0f, 127.0f / 255.0f, 0x7f, 0},
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SINT, -128.0f, 128.0f / 255.0f, 0x80 },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SINT, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SINT, 127.0f, 127.0f, 0x7f, 0},
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SINT, -128.0f, 128.0f, 0x80 },

        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SNORM, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SNORM, 1.0f, 127.0f / 255.0f, 0x7f, 0},
        /* Could be 0x80 or 0x81 */
        { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_SNORM, -1.0f, 129.0f / 255.0f, 0x80, 1},

        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UINT, 0.0f, 0.0f, 0},
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UINT, 255.0f, -1.0f / 127.0f, 0xff, 0},
        /* AMD native drivers return 0x81 here. Seems broken, but NV and Intel do the right thing ... */
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UINT, 128.0f, -1.0f, 0x80, 1 },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UINT, 129.0f, -1.0f, 0x81 },
        { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UINT, 128.0f, -1.0f, 0x80 },
        { DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UINT, 129.0f, -1.0f, 0x81 },

        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UINT, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UINT, 1.0f, 1.0f, 1 },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UINT, 255.0f, -1.0f, 0xff, 0},
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, 1.0f, -1.0f, 0xff, 0},
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, 128.0f / 255.0f, -128.0f, 0x80 },
        { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_UNORM, 129.0f / 255.0f, -127.0f, 0x81 },
#if 0
        /* Not allowed by validation layer. */
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SNORM, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SNORM, 1.0f, 127.0f / 255.0f, 0x7f },
        { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SNORM, -1.0f, 129.0f / 255.0f, 0x80 /* Could be 0x80 or 0x81 */, 1},
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM, 0.0f, 0.0f, 0 },
        { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_UNORM, 1.0f, -1.0f / 127.0f, 0xff },
#endif
    };

    memset(&test_desc, 0, sizeof(test_desc));
    test_desc.no_root_signature = true;
    test_desc.no_pipeline = true;
    test_desc.no_render_target = true;

    if (!init_test_context(&context, &test_desc))
        return;

    if (FAILED(ID3D12Device_CheckFeatureSupport(context.device, D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3))) ||
        !options3.CastingFullyTypedFormatSupported)
    {
        skip("CastingFullyTypedFormat is not supported, skipping ...\n");
        destroy_test_context(&context);
        return;
    }

    heap = create_cpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    for (i = 0; i < 2 * ARRAY_SIZE(tests); i++)
    {
        test_iteration = i / 2;
        fast_clear = !!(i % 2);

        vkd3d_test_set_context("Test %u (%s)", test_iteration, fast_clear ? "fast" : "non-fast");

        memset(&desc, 0, sizeof(desc));
        desc.Width = 1024;
        desc.Height = 1024;
        desc.Format = tests[test_iteration].image;
        desc.DepthOrArraySize = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        desc.MipLevels = 1;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.SampleDesc.Count = 1;

        if (tests[test_iteration].image == DXGI_FORMAT_R8G8B8A8_TYPELESS)
        {
            clear_value.Format = tests[test_iteration].view;
            for (j = 0; j < 4; j++)
                clear_value.Color[j] = tests[test_iteration].clear_value;
        }
        else
        {
            clear_value.Format = tests[test_iteration].image;
            for (j = 0; j < 4; j++)
                clear_value.Color[j] = tests[test_iteration].optimized_clear_value;
        }

        ID3D12Device_CreateCommittedResource(context.device, &heap_properties, D3D12_HEAP_FLAG_NONE,
                &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
                fast_clear ? &clear_value : NULL,
                &IID_ID3D12Resource, (void**)&texture);

        memset(&rtv, 0, sizeof(rtv));
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv.Format = tests[test_iteration].view;
        ID3D12Device_CreateRenderTargetView(context.device, texture, &rtv,
                ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap));

        for (j = 0; j < 4; j++)
            colors[j] = tests[test_iteration].clear_value;

        ID3D12GraphicsCommandList_ClearRenderTargetView(context.list,
                ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap),
                colors, 0, NULL);

        transition_resource_state(context.list, texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

        check_sub_resource_uint(texture, 0, context.queue, context.list, tests[test_iteration].expected_component * 0x01010101u, tests[test_iteration].ulp);

        reset_command_list(context.list, context.allocator);
        ID3D12Resource_Release(texture);
    }
    vkd3d_test_set_context(NULL);

    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_uav_3d_sliced_view(void)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
    D3D12_ROOT_SIGNATURE_DESC rs_desc;
    D3D12_ROOT_PARAMETER root_params[2];
    ID3D12PipelineState *pso_poison;
    ID3D12PipelineState *pso_actual;
    struct resource_readback rb[2];
    D3D12_DESCRIPTOR_RANGE range;
    D3D12_GPU_DESCRIPTOR_HANDLE h;
    uint32_t reference[16][4][4];
    struct test_context context;
    ID3D12DescriptorHeap *heap;
    ID3D12Resource *resource;
    unsigned int x, y, z;
    unsigned int i;

    static const DWORD cs_actual_dxbc[] =
    {
#if 0
    cbuffer C : register(b0) { uint value; }
    RWTexture3D<uint> T : register(u0);

    [numthreads(4, 4, 16)]
    void main(uint3 thr : SV_DispatchThreadID)
    {
        uint w, h, d;
        T.GetDimensions(w, h, d);
        if (thr.z < d)
            T[thr] = value | (w << 8) | (h << 16) | (d << 24);
    }
#endif
        0x43425844, 0xf1736792, 0x8492219a, 0x6751cced, 0xf0219682, 0x00000001, 0x00000188, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x00000134, 0x00050050, 0x0000004d, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0400289c, 0x0011e000, 0x00000000, 0x00004444,
        0x0200005f, 0x00020072, 0x02000068, 0x00000001, 0x0400009b, 0x00000004, 0x00000004, 0x00000010,
        0x8900103d, 0x80000142, 0x00111103, 0x00100072, 0x00000000, 0x00004001, 0x00000000, 0x0011ee46,
        0x00000000, 0x0600004f, 0x00100082, 0x00000000, 0x0002002a, 0x0010002a, 0x00000000, 0x0304001f,
        0x0010003a, 0x00000000, 0x0a000029, 0x00100072, 0x00000000, 0x00100246, 0x00000000, 0x00004002,
        0x00000008, 0x00000010, 0x00000018, 0x00000000, 0x0800003c, 0x00100012, 0x00000000, 0x0010000a,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010001a,
        0x00000000, 0x0010000a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000, 0x0010002a, 0x00000000,
        0x0010000a, 0x00000000, 0x060000a4, 0x0011e0f2, 0x00000000, 0x00020a46, 0x00100006, 0x00000000,
        0x01000015, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE cs_actual = SHADER_BYTECODE(cs_actual_dxbc);

    static const DWORD cs_poison_dxbc[] =
    {
#if 0
    cbuffer C : register(b0) { uint value; }
    RWTexture3D<uint> T : register(u0);

    [numthreads(4, 4, 16)]
    void main(uint3 thr : SV_DispatchThreadID)
    {
            T[thr] = 0xdeadca7;
    }
#endif
        0x43425844, 0x4c99e486, 0x7707bd40, 0xceb3b496, 0xe22f4397, 0x00000001, 0x000000b0, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x0000005c, 0x00050050, 0x00000017, 0x0100086a,
        0x0400289c, 0x0011e000, 0x00000000, 0x00004444, 0x0200005f, 0x00020072, 0x0400009b, 0x00000004,
        0x00000004, 0x00000010, 0x090000a4, 0x0011e0f2, 0x00000000, 0x00020a46, 0x00004002, 0x0deadca7,
        0x0deadca7, 0x0deadca7, 0x0deadca7, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE cs_poison = SHADER_BYTECODE(cs_poison_dxbc);

    static const D3D12_TEX3D_UAV slices[] =
    {
        /* Just to clear everything */
        { 0, 0, -1u }, /* -1 means all remaining slices. */
        { 1, 0, -1u },
        /* ... */

        { 0, 0, 2 },
        { 0, 5, 3 },
        { 0, 9, 1 },
        { 0, 12, 4 },
        { 0, 10, 5 },
        { 1, 0, 2 },
        { 1, 4, 3 },
        { 0, 15, -1u },
        /* WSize = 0 is not allowed. Trips DEVICE_LOST. */
    };

    if (!init_compute_test_context(&context))
        return;

    memset(&rs_desc, 0, sizeof(rs_desc));
    memset(root_params, 0, sizeof(root_params));
    memset(&range, 0, sizeof(range));

    rs_desc.NumParameters = ARRAY_SIZE(root_params);
    rs_desc.pParameters = root_params;

    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_params[0].DescriptorTable.NumDescriptorRanges = 1;
    root_params[0].DescriptorTable.pDescriptorRanges = &range;
    range.NumDescriptors = 1;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_params[1].Constants.Num32BitValues = 1;

    create_root_signature(context.device, &rs_desc, &context.root_signature);
    pso_actual = create_compute_pipeline_state(context.device, context.root_signature, cs_actual);
    pso_poison = create_compute_pipeline_state(context.device, context.root_signature, cs_poison);

    resource = create_default_texture3d(context.device, 4, 4, 16, 2, DXGI_FORMAT_R32_UINT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&uav, 0, sizeof(uav));
    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uav.Format = DXGI_FORMAT_R32_UINT;

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ARRAY_SIZE(slices));

    for (i = 0; i < ARRAY_SIZE(slices); i++)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE h = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
        h.ptr += i * ID3D12Device_GetDescriptorHandleIncrementSize(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        uav.Texture3D = slices[i];
        ID3D12Device_CreateUnorderedAccessView(context.device, resource, NULL, &uav, h);
    }

    ID3D12GraphicsCommandList_SetDescriptorHeaps(context.list, 1, &heap);
    ID3D12GraphicsCommandList_SetComputeRootSignature(context.list, context.root_signature);

    h = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap);

    for (i = 0; i < ARRAY_SIZE(slices); i++)
    {
        ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(context.list, 0, h);
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, i + 1, 0);
        /* First, attempt to flood the descriptor with writes. Validates robustness. */
        ID3D12GraphicsCommandList_SetPipelineState(context.list, pso_poison);
        ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);
        uav_barrier(context.list, resource);
        /* Now, only write in bounds. Makes sure FirstWSlice offset works. */
        ID3D12GraphicsCommandList_SetPipelineState(context.list, pso_actual);
        ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);
        uav_barrier(context.list, resource);

        h.ptr += ID3D12Device_GetDescriptorHandleIncrementSize(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    transition_resource_state(context.list, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    get_texture_readback_with_command_list(resource, 0, &rb[0], context.queue, context.list);

    for (i = 0; i < ARRAY_SIZE(slices); i++)
    {
        unsigned int num_slices;

        if (slices[i].MipSlice != 0)
            continue;

        num_slices = min(16 - slices[i].FirstWSlice, slices[i].WSize);

        for (z = 0; z < num_slices; z++)
        {
            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < 4; x++)
                {
                    uint32_t *ref = &reference[z + slices[i].FirstWSlice][y][x];
                    *ref = i + 1;
                    *ref |= 4 << 8;
                    *ref |= 4 << 16;
                    *ref |= num_slices << 24;
                }
            }
        }
    }

    for (z = 0; z < 16; z++)
    {
        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                uint32_t value;
                value = get_readback_uint(&rb[0], x, y, z);
                todo ok(value == reference[z][y][x], "Error for mip 0 at %u, %u, %u. Got %x, expected %x.\n", x, y, z, value, reference[z][y][x]);
            }
        }
    }

    reset_command_list(context.list, context.allocator);
    get_texture_readback_with_command_list(resource, 1, &rb[1], context.queue, context.list);

    for (i = 0; i < ARRAY_SIZE(slices); i++)
    {
        unsigned int num_slices;

        if (slices[i].MipSlice != 1)
            continue;

        num_slices = min(8 - slices[i].FirstWSlice, slices[i].WSize);

        for (z = 0; z < num_slices; z++)
        {
            for (y = 0; y < 2; y++)
            {
                for (x = 0; x < 2; x++)
                {
                    uint32_t *ref = &reference[z + slices[i].FirstWSlice][y][x];
                    *ref = i + 1;
                    *ref |= 2 << 8;
                    *ref |= 2 << 16;
                    *ref |= num_slices << 24;
                }
            }
        }
    }

    for (z = 0; z < 8; z++)
    {
        for (y = 0; y < 2; y++)
        {
            for (x = 0; x < 2; x++)
            {
                uint32_t value;
                value = get_readback_uint(&rb[1], x, y, z);
                todo ok(value == reference[z][y][x], "Error for mip 1 at %u, %u, %u. Got %x, expected %x.\n", x, y, z, value, reference[z][y][x]);
            }
        }
    }

    for (i = 0; i < ARRAY_SIZE(rb); i++)
        release_resource_readback(&rb[i]);
    ID3D12Resource_Release(resource);
    ID3D12PipelineState_Release(pso_actual);
    ID3D12PipelineState_Release(pso_poison);
    ID3D12DescriptorHeap_Release(heap);
    destroy_test_context(&context);
}

void test_root_descriptor_offset_sign(void)
{
    /* Exploratory test in nature. Will likely crash GPU if not on native drivers. Tweak ifdef to run it. */
#if 1
    skip("Skipping exploratory test for root descriptor over/underflow test.\n");
#else
    ID3D12RootSignature *root_signature;
    D3D12_ROOT_PARAMETER root_params[3];
    D3D12_ROOT_SIGNATURE_DESC rs_desc;
    ID3D12Resource *output_buffer;
    ID3D12Resource *input_buffer;
    struct resource_readback rb;
    struct test_context context;
    ID3D12PipelineState *pso;
    uint32_t values[4];
    unsigned int i;

    static const BYTE cs_code_dxil[] =
    {
#if 0
        RWStructuredBuffer<uint4> RW : register(u0);
        StructuredBuffer<uint> R0 : register(t0);
        ByteAddressBuffer R1 : register(t1);

        [numthreads(1, 1, 1)]
        void main()
        {
                uint a = R0[-1];       // Negative index
                uint b = R0[1u << 30]; // offset 4 GB. Does it overflow back to 0?
                uint c = R1.Load(-4);  // Negative offset
                uint d = R1.Load(0);
                RW[0] = uint4(a, b, c, d);
        }
#endif
        0x44, 0x58, 0x42, 0x43, 0xac, 0xbf, 0xf4, 0x1f, 0x2f, 0x84, 0x34, 0x51, 0x10, 0xd2, 0xe1, 0x21, 0x95, 0x3b, 0xc5, 0x21, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x07, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x90, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe5, 0x8f, 0xa6, 0x7e, 0x5d, 0xa7, 0xe6, 0xd6, 0x02, 0xac, 0xbd, 0xbf, 0x6f, 0x1b, 0xee, 0xc4, 0x44, 0x58, 0x49, 0x4c,
        0xe8, 0x05, 0x00, 0x00, 0x60, 0x00, 0x05, 0x00, 0x7a, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd0, 0x05, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde,
        0x21, 0x0c, 0x00, 0x00, 0x71, 0x01, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39,
        0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62, 0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xa4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88,
        0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90, 0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x29, 0x46, 0x06,
        0x51, 0x18, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xaa, 0x0d, 0x84, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20, 0x01, 0x00, 0x00, 0x00,
        0x49, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04,
        0x93, 0x22, 0xa4, 0x84, 0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c, 0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x60, 0x23, 0x00, 0x25, 0x00, 0x14, 0xe6, 0x08, 0x10, 0x1a, 0xf7,
        0x0c, 0x97, 0x3f, 0x61, 0x0f, 0x21, 0xf9, 0x21, 0xd0, 0x0c, 0x0b, 0x81, 0x02, 0x32, 0x47, 0x00, 0x06, 0x73, 0x04, 0x41, 0x31, 0x8a, 0x19, 0xc6, 0x1c, 0x42, 0x37, 0x0d, 0x97, 0x3f, 0x61, 0x0f,
        0x21, 0xf9, 0x2b, 0x21, 0xad, 0xc4, 0xe4, 0x23, 0xb7, 0x8d, 0x0a, 0x63, 0x8c, 0x31, 0xa5, 0x50, 0xa6, 0x18, 0x43, 0xab, 0x28, 0xc0, 0x14, 0x63, 0x8c, 0x31, 0x66, 0x50, 0x1b, 0x08, 0x98, 0x89,
        0x0c, 0xc6, 0x81, 0x1d, 0xc2, 0x61, 0x1e, 0xe6, 0xc1, 0x0d, 0x66, 0x81, 0x1e, 0xe4, 0xa1, 0x1e, 0xc6, 0x81, 0x1e, 0xea, 0x41, 0x1e, 0xca, 0x81, 0x1c, 0x44, 0xa1, 0x1e, 0xcc, 0xc1, 0x1c, 0xca,
        0x41, 0x1e, 0xf8, 0xa0, 0x1e, 0xdc, 0x61, 0x1e, 0xd2, 0xe1, 0x1c, 0xdc, 0xa1, 0x1c, 0xc8, 0x01, 0x0c, 0xd2, 0xc1, 0x1d, 0xe8, 0xc1, 0x0f, 0x50, 0x60, 0x08, 0x1e, 0x26, 0x4d, 0x11, 0x25, 0x4c,
        0xfe, 0x86, 0x4d, 0x84, 0x36, 0x0c, 0x11, 0x21, 0x49, 0x1b, 0x55, 0x14, 0x44, 0x84, 0x02, 0x43, 0x72, 0x18, 0x81, 0x30, 0x66, 0x92, 0x83, 0x71, 0x60, 0x87, 0x70, 0x98, 0x87, 0x79, 0x70, 0x03,
        0x59, 0xb8, 0x85, 0x59, 0xa0, 0x07, 0x79, 0xa8, 0x87, 0x71, 0xa0, 0x87, 0x7a, 0x90, 0x87, 0x72, 0x20, 0x07, 0x51, 0xa8, 0x07, 0x73, 0x30, 0x87, 0x72, 0x90, 0x07, 0x3e, 0xb0, 0x87, 0x72, 0x18,
        0x07, 0x7a, 0x78, 0x07, 0x79, 0xe0, 0x83, 0x7a, 0x70, 0x87, 0x79, 0x48, 0x87, 0x73, 0x70, 0x87, 0x72, 0x20, 0x07, 0x30, 0x48, 0x07, 0x77, 0xa0, 0x07, 0x36, 0x00, 0x03, 0x3a, 0xf0, 0x03, 0x30,
        0xf0, 0x03, 0x14, 0x50, 0xaa, 0x73, 0x04, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50,
        0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78,
        0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07,
        0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0,
        0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x86, 0x3c, 0x08, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x16, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x34,
        0x40, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x05, 0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11, 0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47,
        0xc6, 0x04, 0x43, 0x32, 0x25, 0x30, 0x02, 0x50, 0x0c, 0x85, 0x51, 0x08, 0x65, 0x51, 0x20, 0x74, 0x46, 0x00, 0xe8, 0x16, 0x08, 0xcd, 0x19, 0x00, 0xb2, 0x33, 0x00, 0x14, 0x67, 0x00, 0x00, 0x00,
        0x79, 0x18, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0x44, 0x35, 0x18, 0x63, 0x0b, 0x73, 0x3b, 0x03, 0xb1, 0x2b, 0x93, 0x9b, 0x4b, 0x7b, 0x73, 0x03, 0x99,
        0x71, 0xb9, 0x01, 0x41, 0xa1, 0x0b, 0x3b, 0x9b, 0x7b, 0x91, 0x2a, 0x62, 0x2a, 0x0a, 0x9a, 0x2a, 0xfa, 0x9a, 0xb9, 0x81, 0x79, 0x31, 0x4b, 0x73, 0x0b, 0x63, 0x4b, 0xd9, 0x10, 0x04, 0x13, 0x04,
        0x83, 0x98, 0x20, 0x18, 0xc5, 0x06, 0x61, 0x20, 0x26, 0x08, 0x86, 0xb1, 0x41, 0x18, 0x0c, 0x0a, 0x63, 0x73, 0x1b, 0x06, 0xc4, 0x20, 0x26, 0x08, 0xd1, 0x43, 0x60, 0x82, 0x60, 0x1c, 0x13, 0x04,
        0x03, 0xd9, 0x20, 0x0c, 0xcd, 0x86, 0x84, 0x50, 0x16, 0x82, 0x18, 0x18, 0xc2, 0x99, 0x20, 0x4c, 0xcd, 0x04, 0xc1, 0x48, 0x36, 0x24, 0x03, 0xb4, 0x10, 0xc3, 0x10, 0x11, 0xc0, 0x06, 0xe1, 0x91,
        0x26, 0x08, 0x96, 0x33, 0x41, 0x38, 0x96, 0x09, 0x82, 0xa1, 0x6c, 0x10, 0x06, 0x6b, 0xc3, 0x42, 0x50, 0x0b, 0x41, 0x0c, 0x4c, 0x55, 0x55, 0xd7, 0x86, 0x00, 0xdb, 0x40, 0x4c, 0x19, 0x00, 0x4c,
        0x10, 0x04, 0x80, 0x44, 0x5b, 0x58, 0x9a, 0xdb, 0x04, 0xe1, 0x62, 0x36, 0x0c, 0xc3, 0x30, 0x6c, 0x20, 0x88, 0xae, 0xf1, 0x36, 0x14, 0x1b, 0x07, 0x68, 0x5f, 0x15, 0x36, 0x36, 0xbb, 0x36, 0x97,
        0x34, 0xb2, 0x32, 0x37, 0xba, 0x29, 0x41, 0x50, 0x85, 0x0c, 0xcf, 0xc5, 0xae, 0x4c, 0x6e, 0x2e, 0xed, 0xcd, 0x6d, 0x4a, 0x40, 0x34, 0x21, 0xc3, 0x73, 0xb1, 0x0b, 0x63, 0xb3, 0x2b, 0x93, 0x9b,
        0x12, 0x18, 0x75, 0xc8, 0xf0, 0x5c, 0xe6, 0xd0, 0xc2, 0xc8, 0xca, 0xe4, 0x9a, 0xde, 0xc8, 0xca, 0xd8, 0xa6, 0x04, 0x48, 0x19, 0x32, 0x3c, 0x17, 0xb9, 0xb2, 0xb9, 0xb7, 0x3a, 0xb9, 0xb1, 0xb2,
        0xb9, 0x29, 0x41, 0x56, 0x87, 0x0c, 0xcf, 0xa5, 0xcc, 0x8d, 0x4e, 0x2e, 0x0f, 0xea, 0x2d, 0xcd, 0x8d, 0x6e, 0x6e, 0x4a, 0xf0, 0x01, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00,
        0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10,
        0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03,
        0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20, 0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e,
        0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b,
        0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90,
        0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e,
        0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca,
        0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82,
        0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x8c, 0xc8, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x72, 0x10, 0x87, 0x73, 0x70, 0x03, 0x7b, 0x08, 0x07, 0x79, 0x60, 0x87, 0x70, 0xc8, 0x87, 0x77, 0xa8, 0x07, 0x7a,
        0x00, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x26, 0x40, 0x0d, 0x97, 0xef, 0x3c, 0x7e, 0x40, 0x15, 0x05, 0x11, 0x95, 0x0e, 0x30, 0xf8, 0xc8, 0x6d, 0xdb, 0x40, 0x35,
        0x5c, 0xbe, 0xf3, 0xf8, 0x01, 0x55, 0x14, 0x44, 0xc4, 0x4e, 0x4e, 0x44, 0xf8, 0xc8, 0x6d, 0x5b, 0x80, 0x34, 0x5c, 0xbe, 0xf3, 0xf8, 0x42, 0x44, 0x00, 0x13, 0x11, 0x02, 0xcd, 0xb0, 0x10, 0x06,
        0x40, 0x30, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x00, 0x61, 0x20, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x13, 0x04, 0x41, 0x2c, 0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x34, 0x46, 0x00, 0x4a,
        0xa0, 0x3c, 0xc8, 0x14, 0x62, 0x40, 0xc9, 0xcd, 0x00, 0xd4, 0x40, 0x01, 0x02, 0x02, 0x02, 0x22, 0x54, 0x42, 0x29, 0x06, 0x00, 0x00, 0x00, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0x80, 0x64,
        0x46, 0x52, 0x55, 0xcf, 0x88, 0x41, 0x02, 0x80, 0x20, 0x18, 0x20, 0xda, 0xb1, 0x5c, 0x17, 0x34, 0x62, 0x90, 0x00, 0x20, 0x08, 0x06, 0xc8, 0x86, 0x30, 0xd7, 0x15, 0x8d, 0x18, 0x20, 0x00, 0x08,
        0x82, 0xc1, 0xb2, 0x29, 0xc1, 0x81, 0x8d, 0x26, 0x04, 0xc0, 0x88, 0x01, 0x02, 0x80, 0x20, 0x18, 0x2c, 0x1d, 0x33, 0x20, 0xda, 0x68, 0x42, 0x00, 0x8c, 0x18, 0x20, 0x00, 0x08, 0x82, 0xc1, 0xf2,
        0x39, 0x46, 0xc2, 0x8c, 0x26, 0x04, 0xc0, 0x88, 0x01, 0x02, 0x80, 0x20, 0x18, 0x2c, 0x61, 0x00, 0x21, 0x9e, 0x33, 0x9a, 0x10, 0x00, 0x23, 0x06, 0x0d, 0x00, 0x82, 0x60, 0xd0, 0x88, 0x01, 0xb3,
        0x80, 0x01, 0x18, 0x1c, 0xc5, 0x10, 0x4c, 0x08, 0x00, 0x00, 0x00, 0x00,
    };
    static const D3D12_SHADER_BYTECODE cs_code = SHADER_BYTECODE(cs_code_dxil);

    static const uint32_t test_data[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    memset(&rs_desc, 0, sizeof(rs_desc));
    rs_desc.NumParameters = ARRAY_SIZE(root_params);
    rs_desc.pParameters = root_params;
    memset(root_params, 0, sizeof(root_params));
    root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    root_params[2].Descriptor.ShaderRegister = 1;


    if (!init_compute_test_context(&context))
        return;

    if (!context_supports_dxil(&context))
    {
        destroy_test_context(&context);
        return;
    }

    input_buffer = create_upload_buffer(context.device, sizeof(test_data), test_data);
    output_buffer = create_default_buffer(context.device, 16,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    create_root_signature(context.device, &rs_desc, &root_signature);
    pso = create_compute_pipeline_state(context.device, root_signature, cs_code);
    ID3D12GraphicsCommandList_SetPipelineState(context.list, pso);
    ID3D12GraphicsCommandList_SetComputeRootSignature(context.list, root_signature);
    ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(context.list, 0,
            ID3D12Resource_GetGPUVirtualAddress(output_buffer));
    ID3D12GraphicsCommandList_SetComputeRootShaderResourceView(context.list, 1,
            ID3D12Resource_GetGPUVirtualAddress(input_buffer) + 16);
    ID3D12GraphicsCommandList_SetComputeRootShaderResourceView(context.list, 2,
            ID3D12Resource_GetGPUVirtualAddress(input_buffer) + 16);
    ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);
    transition_resource_state(context.list, output_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(output_buffer, DXGI_FORMAT_R32_UINT, &rb, context.queue, context.list);

    for (i = 0; i < 4; i++)
        values[i] = get_readback_uint(&rb, i, 0, 0);

    skip("Got structured access [-1] = #%x\n", values[0]);
    skip("Got structured access [1u << 30] = #%x\n", values[1]);
    skip("Got byte address [-4] = #%x\n", values[2]);
    skip("Got byte address [0] = #%x\n", values[3]);

    /* Observed on AMD:
    test_root_descriptor_offset_sign:5262: Test skipped: Got structured access [-1] = #4b416743 <-- Garbage. Likely we accessed garbage memory way out at (4 * UINT_MAX) & UINT_MAX offset.
    test_root_descriptor_offset_sign:5263: Test skipped: Got structured access [1u << 30] = #4 <-- Suggests 32-bit uint offset.
    test_root_descriptor_offset_sign:5264: Test skipped: Got byte address [-4] = #0 <-- Suggests we hit robustness for driver generated descriptor.
    test_root_descriptor_offset_sign:5265: Test skipped: Got byte address [0] = #4
    */

    /* Observed on NV: Blue screen of death (?!?!). */

    /* Observed on Intel: All 0. Likely faulted and terminated the dispatch before we could write results. */

    ID3D12RootSignature_Release(root_signature);
    ID3D12PipelineState_Release(pso);
    ID3D12Resource_Release(input_buffer);
    ID3D12Resource_Release(output_buffer);
    release_resource_readback(&rb);
    destroy_test_context(&context);
#endif
}

static void test_uav_counters_null_behavior(bool use_dxil)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_ROOT_SIGNATURE_DESC rs_desc;
    ID3D12DescriptorHeap *cpu_heap;
    D3D12_ROOT_PARAMETER rs_param;
    D3D12_DESCRIPTOR_RANGE range;
    struct test_context context;
    struct resource_readback rb;
    ID3D12DescriptorHeap *heap;
    ID3D12Resource *resource;
    unsigned int i;

#if 0
    RWStructuredBuffer<uint> RWBuf[4] : register(u0);

    [numthreads(1, 1, 1)]
    void main(int wg : SV_GroupID)
    {
        RWBuf[wg >> 2][wg & 3] = RWBuf[wg >> 2].IncrementCounter() + 64;
    }
#endif
    static const DWORD cs_code_dxbc[] =
    {
        0x43425844, 0xb5433247, 0x4cd30f6c, 0x58100e67, 0xc179ade1, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x000000e0, 0x00050051, 0x00000038, 0x0100086a,
        0x0700009e, 0x0031ee46, 0x00000000, 0x00000000, 0x00000003, 0x00000004, 0x00000000, 0x0200005f,
        0x00021012, 0x02000068, 0x00000002, 0x0400009b, 0x00000001, 0x00000001, 0x00000001, 0x0600002a,
        0x00100012, 0x00000000, 0x0002100a, 0x00004001, 0x00000002, 0x06000001, 0x00100022, 0x00000000,
        0x0002100a, 0x00004001, 0x00000003, 0x070000b2, 0x00100012, 0x00000001, 0x0421e000, 0x00000000,
        0x0010000a, 0x00000000, 0x0700001e, 0x00100042, 0x00000000, 0x0010000a, 0x00000001, 0x00004001,
        0x00000040, 0x0b0000a8, 0x0421e012, 0x00000000, 0x0010000a, 0x00000000, 0x0010001a, 0x00000000,
        0x00004001, 0x00000000, 0x0010002a, 0x00000000, 0x0100003e,
    };

    static const BYTE cs_code_dxil[] =
    {
        0x44, 0x58, 0x42, 0x43, 0xc6, 0xfe, 0xe1, 0x77, 0xd8, 0x5c, 0x56, 0xc7, 0x6e, 0xf7, 0xe2, 0xf7, 0xb3, 0xb0, 0x40, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x34, 0x06, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x60, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0xc1, 0xf5, 0xe2,
        0x29, 0x0a, 0x7c, 0x68, 0x4a, 0xfa, 0x15, 0xe9, 0x1a, 0x85, 0x63, 0x21, 0x44, 0x58, 0x49, 0x4c, 0x40, 0x05, 0x00, 0x00, 0x60, 0x00, 0x05, 0x00, 0x50, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
        0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x28, 0x05, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0x47, 0x01, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62, 0x80, 0x14, 0x45, 0x02,
        0x42, 0x92, 0x0b, 0x42, 0xa4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88, 0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90,
        0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07,
        0x40, 0x02, 0xa8, 0x0d, 0x84, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20, 0x6d, 0x30, 0x86, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x09, 0xa8, 0x00, 0x49, 0x18, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x13, 0x82, 0x60, 0x42, 0x20, 0x4c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04, 0x93, 0x22, 0xa4, 0x84,
        0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c, 0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x54, 0x23, 0x00, 0x25, 0x00, 0x14, 0xe6, 0x08, 0xc0, 0xa0, 0x0c, 0x63, 0x0c, 0x22, 0x73,
        0x04, 0x08, 0x99, 0x7b, 0x86, 0xcb, 0x9f, 0xb0, 0x87, 0x90, 0xfc, 0x10, 0x68, 0x86, 0x85, 0x40, 0xc1, 0x29, 0xc4, 0x18, 0xc8, 0x50, 0x9a, 0x23, 0x08, 0x8a, 0x81, 0x86, 0x19, 0x63, 0x11, 0x2b,
        0x0a, 0x18, 0x68, 0x8c, 0x31, 0xc6, 0x30, 0xe4, 0x06, 0x02, 0x66, 0x32, 0x83, 0x71, 0x60, 0x87, 0x70, 0x98, 0x87, 0x79, 0x70, 0x03, 0x59, 0xb8, 0x85, 0x59, 0xa0, 0x07, 0x79, 0xa8, 0x87, 0x71,
        0xa0, 0x87, 0x7a, 0x90, 0x87, 0x72, 0x20, 0x07, 0x51, 0xa8, 0x07, 0x73, 0x30, 0x87, 0x72, 0x90, 0x07, 0x3e, 0xa8, 0x07, 0x77, 0x98, 0x87, 0x74, 0x38, 0x07, 0x77, 0x28, 0x07, 0x72, 0x00, 0x83,
        0x74, 0x70, 0x07, 0x7a, 0xf0, 0x03, 0x14, 0x8c, 0x24, 0x88, 0x24, 0xe7, 0x08, 0x40, 0x01, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0,
        0x87, 0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07,
        0x6d, 0x90, 0x0e, 0x78, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90,
        0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6,
        0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x12, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x18, 0xf2, 0x30, 0x40, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4, 0x71, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x04, 0x00, 0x00,
        0x09, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x10, 0x19, 0x11, 0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x25, 0x30, 0x02, 0x50, 0x0c, 0x85, 0x50, 0x18, 0xb4, 0x46, 0x00, 0x6a,
        0x80, 0x68, 0x81, 0xd0, 0x9c, 0x01, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0x44, 0x35, 0x18, 0x63, 0x0b, 0x73, 0x3b, 0x03, 0xb1,
        0x2b, 0x93, 0x9b, 0x4b, 0x7b, 0x73, 0x03, 0x99, 0x71, 0xb9, 0x01, 0x41, 0xa1, 0x0b, 0x3b, 0x9b, 0x7b, 0x91, 0x2a, 0x62, 0x2a, 0x0a, 0x9a, 0x2a, 0xfa, 0x9a, 0xb9, 0x81, 0x79, 0x31, 0x4b, 0x73,
        0x0b, 0x63, 0x4b, 0xd9, 0x10, 0x04, 0x13, 0x84, 0xa1, 0x98, 0x20, 0x0c, 0xc6, 0x06, 0x61, 0x20, 0x26, 0x08, 0xc3, 0xb1, 0x41, 0x18, 0x0c, 0x0a, 0x63, 0x73, 0x1b, 0x06, 0xc4, 0x20, 0x26, 0x08,
        0x53, 0x43, 0x60, 0x82, 0x30, 0x20, 0x13, 0x84, 0x21, 0x99, 0x20, 0x2c, 0xca, 0x04, 0x61, 0x59, 0x36, 0x08, 0x03, 0xb3, 0x61, 0x21, 0x94, 0x85, 0x20, 0x98, 0xc6, 0x79, 0x1c, 0x68, 0x43, 0x10,
        0x6d, 0x20, 0x00, 0x09, 0x00, 0x26, 0x08, 0x02, 0x40, 0xa2, 0x2d, 0x2c, 0xcd, 0x6d, 0x82, 0x40, 0x31, 0x1b, 0x86, 0x61, 0x18, 0x36, 0x10, 0x84, 0xc5, 0x5c, 0x1b, 0x0a, 0xaa, 0x02, 0x26, 0xac,
        0x0a, 0x1b, 0x9b, 0x5d, 0x9b, 0x4b, 0x1a, 0x59, 0x99, 0x1b, 0xdd, 0x94, 0x20, 0xa8, 0x42, 0x86, 0xe7, 0x62, 0x57, 0x26, 0x37, 0x97, 0xf6, 0xe6, 0x36, 0x25, 0x20, 0x9a, 0x90, 0xe1, 0xb9, 0xd8,
        0x85, 0xb1, 0xd9, 0x95, 0xc9, 0x4d, 0x09, 0x8c, 0x3a, 0x64, 0x78, 0x2e, 0x73, 0x68, 0x61, 0x64, 0x65, 0x72, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x53, 0x02, 0xa4, 0x0c, 0x19, 0x9e, 0x8b, 0x5c, 0xd9,
        0xdc, 0x5b, 0x9d, 0xdc, 0x58, 0xd9, 0xdc, 0x94, 0x40, 0xaa, 0x43, 0x86, 0xe7, 0x52, 0xe6, 0x46, 0x27, 0x97, 0x07, 0xf5, 0x96, 0xe6, 0x46, 0x37, 0x37, 0x25, 0xc0, 0x00, 0x79, 0x18, 0x00, 0x00,
        0x4c, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6,
        0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8,
        0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20, 0x87, 0x19, 0xcc, 0x11,
        0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89,
        0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37,
        0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81,
        0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c,
        0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc,
        0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19, 0xd1, 0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20,
        0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x46, 0x50, 0x0d, 0x97,
        0xef, 0x3c, 0x7e, 0x40, 0x15, 0x05, 0x11, 0xb1, 0x93, 0x13, 0x11, 0x3e, 0x72, 0xdb, 0x26, 0x90, 0x0d, 0x97, 0xef, 0x3c, 0x7e, 0x40, 0x15, 0x05, 0x11, 0xb9, 0xcf, 0x00, 0x4c, 0x04, 0xe7, 0x50,
        0xcd, 0x44, 0x44, 0x36, 0x20, 0x0d, 0x97, 0xef, 0x3c, 0xbe, 0x10, 0x11, 0xc0, 0x44, 0x84, 0x40, 0x33, 0x2c, 0x84, 0x05, 0x44, 0xc3, 0xe5, 0x3b, 0x8f, 0x6f, 0x44, 0x0e, 0xf5, 0x88, 0x83, 0x8f,
        0xdc, 0xb6, 0x01, 0x10, 0x0c, 0x80, 0x34, 0x00, 0x61, 0x20, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x13, 0x04, 0x41, 0x2c, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x34, 0x66, 0x00, 0x8a,
        0x37, 0xa0, 0x08, 0x4a, 0xae, 0x18, 0x03, 0x0a, 0x30, 0xa0, 0x0c, 0x4a, 0x31, 0x80, 0x4c, 0x09, 0x00, 0x00, 0x00, 0x00, 0x23, 0x06, 0x06, 0x00, 0x82, 0x60, 0x40, 0x58, 0x48, 0x54, 0x01, 0x92,
        0x15, 0x4c, 0x30, 0x62, 0x90, 0x00, 0x20, 0x08, 0x06, 0xcc, 0x95, 0x10, 0x54, 0x00, 0x8d, 0x18, 0x1c, 0x00, 0x08, 0x82, 0x41, 0x92, 0x25, 0x41, 0x51, 0x41, 0x02, 0x65, 0x24, 0x3a, 0x62, 0xd0,
        0x00, 0x20, 0x08, 0x06, 0x8e, 0x96, 0x10, 0x01, 0x26, 0x40, 0x10, 0x84, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    static const D3D12_SHADER_BYTECODE cs_dxbc = SHADER_BYTECODE(cs_code_dxbc);
    static const D3D12_SHADER_BYTECODE cs_dxil = SHADER_BYTECODE(cs_code_dxil);

    if (!init_compute_test_context(&context))
        return;

    if (use_dxil && !context_supports_dxil(&context))
    {
        skip("Context does not support DXIL.\n");
        destroy_test_context(&context);
        return;
    }

    memset(&rs_desc, 0, sizeof(rs_desc));
    memset(&rs_param, 0, sizeof(rs_param));
    memset(&range, 0, sizeof(range));
    rs_desc.NumParameters = 1;
    rs_desc.pParameters = &rs_param;
    rs_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rs_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rs_param.DescriptorTable.NumDescriptorRanges = 1;
    rs_param.DescriptorTable.pDescriptorRanges = &range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 8;
    create_root_signature(context.device, &rs_desc, &context.root_signature);
    context.pipeline_state = create_compute_pipeline_state(context.device, context.root_signature, use_dxil ? cs_dxil : cs_dxbc);

    cpu_heap = create_cpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8);
    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8);
    resource = create_default_buffer(context.device, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * 9,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    for (i = 0; i < 8; i++)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_h, gpu_h;
        cpu_h = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(cpu_heap);
        gpu_h = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
        cpu_h.ptr += ID3D12Device_GetDescriptorHandleIncrementSize(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;
        gpu_h.ptr += ID3D12Device_GetDescriptorHandleIncrementSize(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;

        uav_desc.Buffer.NumElements = 4;
        uav_desc.Buffer.FirstElement = 4 * i;
        uav_desc.Buffer.StructureByteStride = 4;
        uav_desc.Buffer.CounterOffsetInBytes = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * (i + 1);

        /* AMD drivers don't seem to clear the UAV counter if we pass in NULL, so
         * test a path which does not do that. */
        if (i < 4)
        {
            ID3D12Device_CreateUnorderedAccessView(context.device, resource, resource, &uav_desc, cpu_h);
            ID3D12Device_CreateUnorderedAccessView(context.device, resource, resource, &uav_desc, gpu_h);
        }

        uav_desc.Buffer.CounterOffsetInBytes = 0;

        /* Test writing NULL UAV counter after a non-NULL UAV counter. Makes sure that we are indeed supposed
         * to clear out UAV counters to NULL every time. */
        if ((i & 3) == 3)
        {
            ID3D12Device_CreateUnorderedAccessView(context.device, NULL, NULL, &uav_desc, cpu_h);
            ID3D12Device_CreateUnorderedAccessView(context.device, NULL, NULL, &uav_desc, gpu_h);
        }
        else if ((i & 3) >= 1)
        {
            ID3D12Device_CreateUnorderedAccessView(context.device, resource, NULL, &uav_desc, cpu_h);
            ID3D12Device_CreateUnorderedAccessView(context.device, resource, NULL, &uav_desc, gpu_h);
        }
        else
        {
            /* Test copy behavior. Make sure we correctly copy NULL counters as well. */
            ID3D12Device_CreateUnorderedAccessView(context.device, resource, NULL, &uav_desc, cpu_h);
            ID3D12Device_CopyDescriptorsSimple(context.device, 1,
                       gpu_h, cpu_h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }

    ID3D12GraphicsCommandList_SetDescriptorHeaps(context.list, 1, &heap);
    ID3D12GraphicsCommandList_SetComputeRootSignature(context.list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(context.list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(context.list, 0,
              ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));
    ID3D12GraphicsCommandList_Dispatch(context.list, 8 * 4, 1, 1);
    transition_resource_state(context.list, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(resource, DXGI_FORMAT_R32_UINT, &rb, context.queue, context.list);

    for (i = 0; i < 8 * 4; i++)
    {
        /* Possible behavior is very varied here:
         * NV: If UAV counter is NULL, NV makes the main descriptor robust.
         * AMD: Writing NULL uav counter does not update the counter descriptor, the atomic update will still go through.
         * Intel: Behaves as you would expect. Atomic op returns 0, writes to main descriptor behaves as you'd expect. */
        uint32_t value = get_readback_uint(&rb, i, 0, 0);
        ok(value == 0 || (value >= 64 && value < (64 + 4)), "Unexpected value %u = %u\n", i, value);
    }

    for (i = 0; i < 8; i++)
    {
        uint32_t value = get_readback_uint(&rb, (i + 1) * (D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT / 4), 0, 0);
        if (i < 4)
        {
            /* AMD behavior: Passing NULL does not necessarily clear out UAV counter.
             * It is undefined to access UAV counter like this.
             * https://docs.microsoft.com/en-us/windows/win32/direct3d12/uav-counters
             * "If a shader attempts to access the counter of a UAV that does not have an associated counter,
             *  then the debug layer will issue a warning,
             *  and a GPU page fault will occur causing the apps’s device to be removed." */
            ok(value == 0 || value == 4, "Unexpected counter %u = %u.\n", i, value);
        }
        else
        {
            /* Technically undefined, but all drivers behave robustly here, we should too. */
            ok(value == 0, "Unexpected counter %u = %u.\n", i, value);
        }
    }

    release_resource_readback(&rb);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12DescriptorHeap_Release(cpu_heap);
    ID3D12Resource_Release(resource);
    destroy_test_context(&context);
}

void test_uav_counter_null_behavior_dxbc(void)
{
    test_uav_counters_null_behavior(false);
}

void test_uav_counter_null_behavior_dxil(void)
{
    test_uav_counters_null_behavior(true);
}

static void test_uav_robustness_oob_structure_element(bool use_dxil)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    D3D12_CPU_DESCRIPTOR_HANDLE gpu_h;
    D3D12_ROOT_SIGNATURE_DESC rs_desc;
    D3D12_ROOT_PARAMETER rs_params[2];
    D3D12_DESCRIPTOR_RANGE range;
    struct test_context context;
    struct resource_readback rb;
    ID3D12DescriptorHeap *heap;
    ID3D12Resource *resource;
    unsigned int i;

#if 0
    struct A { uint arr[4]; };
    RWStructuredBuffer<A> U0 : register(u0);

    cbuffer Cbuf : register(b0) { uint elem; uint index; uint value; };

    [numthreads(1, 1, 1)]
    void main()
    {
        U0[elem].arr[index] = value;
    }
#endif
    static const DWORD cs_code_dxbc[] =
    {
        0x43425844, 0xf2b14418, 0xe00c2b39, 0x9534ef2d, 0x063df3b0, 0x00000001, 0x000000e8, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x58454853, 0x00000094, 0x00050050, 0x00000025, 0x0100086a,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0400009e, 0x0011e000, 0x00000000, 0x00000010,
        0x02000068, 0x00000001, 0x0400009b, 0x00000001, 0x00000001, 0x00000001, 0x08000029, 0x00100012,
        0x00000000, 0x0020801a, 0x00000000, 0x00000000, 0x00004001, 0x00000002, 0x0b0000a8, 0x0011e012,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x0010000a, 0x00000000, 0x0020802a, 0x00000000,
        0x00000000, 0x0100003e,
    };

    static const BYTE cs_code_dxil[] =
    {
        0x44, 0x58, 0x42, 0x43, 0xa0, 0xfc, 0x75, 0xac, 0xa3, 0x6f, 0xcd, 0xfc, 0xba, 0x55, 0x03, 0x63, 0x6e, 0xfa, 0xc8, 0x44, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x06, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x78, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x2e, 0xe7, 0xe7, 0x1b, 0x84, 0x73, 0x5f, 0xb8, 0xe9, 0x8b, 0x7f,
        0x49, 0x16, 0x15, 0xed, 0x44, 0x58, 0x49, 0x4c, 0x30, 0x05, 0x00, 0x00, 0x60, 0x00, 0x05, 0x00, 0x4c, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x18, 0x05, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0x43, 0x01, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91,
        0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62, 0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xa4, 0x10, 0x32, 0x14,
        0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88, 0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90, 0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c,
        0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xaa, 0x0d, 0x84, 0xf0, 0xff, 0xff,
        0xff, 0xff, 0x03, 0x20, 0x01, 0x00, 0x00, 0x00, 0x49, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00,
        0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04, 0x93, 0x22, 0xa4, 0x84, 0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c, 0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x5c, 0x23, 0x00,
        0x25, 0x00, 0x14, 0xe6, 0x08, 0xc0, 0xe0, 0xa8, 0xe1, 0xf2, 0x27, 0xec, 0x21, 0x24, 0x9f, 0xdb, 0xa8, 0x62, 0x25, 0x26, 0x1f, 0xb9, 0x6d, 0x44, 0x8c, 0x31, 0xc6, 0x1c, 0x01, 0x42, 0xe5, 0x9e,
        0xe1, 0xf2, 0x27, 0xec, 0x21, 0x24, 0x3f, 0x04, 0x9a, 0x61, 0x21, 0x50, 0x60, 0x0a, 0x41, 0xc6, 0x19, 0x84, 0xe6, 0x08, 0x82, 0x62, 0x9c, 0x51, 0xc6, 0x50, 0xb4, 0x8a, 0x02, 0xc6, 0x19, 0x63,
        0x8c, 0x31, 0x0a, 0xb5, 0x81, 0x80, 0x24, 0x18, 0x87, 0x48, 0x53, 0x44, 0x09, 0x93, 0xaf, 0xa1, 0x00, 0xce, 0xb4, 0x8d, 0x03, 0x3b, 0x84, 0xc3, 0x3c, 0xcc, 0x83, 0x1b, 0xc8, 0xc2, 0x2d, 0xcc,
        0x02, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xd4, 0x83, 0x3c, 0x94, 0x03, 0x39, 0x88, 0x42, 0x3d, 0x98, 0x83, 0x39, 0x94, 0x83, 0x3c, 0xf0, 0x41, 0x28, 0xf8, 0x01, 0x0a, 0x22, 0xc9, 0x23,
        0xb8, 0x80, 0x2a, 0xd0, 0x30, 0xc6, 0x20, 0x3a, 0x47, 0x00, 0x0a, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50,
        0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78,
        0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07,
        0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0,
        0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x86, 0x3c, 0x08, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x16, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x34,
        0x40, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x05, 0x02, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x10, 0x19, 0x11, 0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47,
        0xc6, 0x04, 0x43, 0x1a, 0x25, 0x30, 0x02, 0x50, 0x0c, 0x85, 0x51, 0x20, 0x85, 0x40, 0x6a, 0x04, 0x80, 0x6c, 0x81, 0x50, 0x9d, 0x01, 0xa0, 0x39, 0x03, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00,
        0x3f, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0x44, 0x35, 0x18, 0x63, 0x0b, 0x73, 0x3b, 0x03, 0xb1, 0x2b, 0x93, 0x9b, 0x4b, 0x7b, 0x73, 0x03, 0x99, 0x71, 0xb9, 0x01, 0x41,
        0xa1, 0x0b, 0x3b, 0x9b, 0x7b, 0x91, 0x2a, 0x62, 0x2a, 0x0a, 0x9a, 0x2a, 0xfa, 0x9a, 0xb9, 0x81, 0x79, 0x31, 0x4b, 0x73, 0x0b, 0x63, 0x4b, 0xd9, 0x10, 0x04, 0x13, 0x84, 0x81, 0x98, 0x20, 0x0c,
        0xc5, 0x06, 0x61, 0x20, 0x26, 0x08, 0x83, 0xb1, 0x41, 0x18, 0x0c, 0x0a, 0x63, 0x73, 0x1b, 0x06, 0xc4, 0x20, 0x26, 0x08, 0x53, 0x43, 0x60, 0x82, 0x30, 0x1c, 0x13, 0x04, 0x45, 0x99, 0x20, 0x0c,
        0xc8, 0x06, 0x61, 0x70, 0x36, 0x2c, 0x84, 0xb2, 0x10, 0xc4, 0xc0, 0x34, 0x4d, 0xf3, 0x6c, 0x08, 0xa0, 0x09, 0x42, 0xc5, 0x6c, 0x40, 0x08, 0x69, 0x21, 0x88, 0x81, 0x01, 0x36, 0x04, 0xd3, 0x06,
        0x02, 0x88, 0x28, 0x60, 0x82, 0x20, 0x00, 0x24, 0xda, 0xc2, 0xd2, 0xdc, 0x26, 0x08, 0xd6, 0x32, 0x41, 0x18, 0x92, 0x0d, 0xc3, 0x30, 0x0c, 0x1b, 0x08, 0x02, 0xcb, 0xb4, 0x0d, 0x85, 0x75, 0x01,
        0xd5, 0x56, 0x85, 0x8d, 0xcd, 0xae, 0xcd, 0x25, 0x8d, 0xac, 0xcc, 0x8d, 0x6e, 0x4a, 0x10, 0x54, 0x21, 0xc3, 0x73, 0xb1, 0x2b, 0x93, 0x9b, 0x4b, 0x7b, 0x73, 0x9b, 0x12, 0x10, 0x4d, 0xc8, 0xf0,
        0x5c, 0xec, 0xc2, 0xd8, 0xec, 0xca, 0xe4, 0xa6, 0x04, 0x46, 0x1d, 0x32, 0x3c, 0x97, 0x39, 0xb4, 0x30, 0xb2, 0x32, 0xb9, 0xa6, 0x37, 0xb2, 0x32, 0xb6, 0x29, 0x01, 0x52, 0x86, 0x0c, 0xcf, 0x45,
        0xae, 0x6c, 0xee, 0xad, 0x4e, 0x6e, 0xac, 0x6c, 0x6e, 0x4a, 0x40, 0xd5, 0x21, 0xc3, 0x73, 0x29, 0x73, 0xa3, 0x93, 0xcb, 0x83, 0x7a, 0x4b, 0x73, 0xa3, 0x9b, 0x9b, 0x12, 0x6c, 0x00, 0x00, 0x00,
        0x79, 0x18, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73,
        0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b,
        0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20,
        0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61,
        0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87,
        0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98,
        0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61,
        0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b,
        0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x8c, 0xc8, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x72, 0x10, 0x87, 0x73, 0x70, 0x03, 0x7b, 0x08, 0x07, 0x79, 0x60,
        0x87, 0x70, 0xc8, 0x87, 0x77, 0xa8, 0x07, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x36, 0x50, 0x0d, 0x97, 0xef, 0x3c, 0x7e, 0x40, 0x15, 0x05, 0x11, 0xb1,
        0x93, 0x13, 0x11, 0x3e, 0x72, 0xdb, 0x16, 0xb0, 0x0d, 0x97, 0xef, 0x3c, 0xbe, 0x10, 0x50, 0x45, 0x41, 0x44, 0xa5, 0x03, 0x0c, 0x25, 0x61, 0x00, 0x02, 0xe6, 0x23, 0xb7, 0x6d, 0x02, 0xd2, 0x70,
        0xf9, 0xce, 0xe3, 0x0b, 0x11, 0x01, 0x4c, 0x44, 0x08, 0x34, 0xc3, 0x42, 0x18, 0x00, 0xc1, 0x00, 0x48, 0x03, 0x00, 0x00, 0x61, 0x20, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x13, 0x04, 0x41, 0x2c,
        0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x34, 0x66, 0x00, 0x4a, 0xae, 0xec, 0x8a, 0xa0, 0x14, 0x03, 0xa8, 0x94, 0x40, 0x11, 0x00, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0xb0, 0x4c,
        0x86, 0x00, 0x41, 0xcb, 0x88, 0x41, 0x02, 0x80, 0x20, 0x18, 0x2c, 0xd4, 0x21, 0x44, 0x11, 0x33, 0x62, 0x70, 0x00, 0x20, 0x08, 0x06, 0x88, 0x75, 0x04, 0xd2, 0x68, 0x42, 0x20, 0x8c, 0x26, 0x08,
        0xc1, 0x68, 0xc2, 0x00, 0x94, 0xa0, 0xdc, 0x88, 0x41, 0x03, 0x80, 0x20, 0x18, 0x34, 0x99, 0x72, 0x08, 0x01, 0xe1, 0x38, 0x4e, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    static const D3D12_SHADER_BYTECODE cs_dxil = SHADER_BYTECODE(cs_code_dxil);
    static const D3D12_SHADER_BYTECODE cs_dxbc = SHADER_BYTECODE(cs_code_dxbc);

    if (!init_compute_test_context(&context))
        return;

    if (use_dxil && !context_supports_dxil(&context))
    {
        skip("Context does not support DXIL.\n");
        destroy_test_context(&context);
        return;
    }

    memset(&rs_desc, 0, sizeof(rs_desc));
    memset(rs_params, 0, sizeof(rs_params));
    memset(&range, 0, sizeof(range));
    rs_desc.NumParameters = ARRAY_SIZE(rs_params);
    rs_desc.pParameters = rs_params;
    rs_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rs_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rs_params[0].DescriptorTable.NumDescriptorRanges = 1;
    rs_params[0].DescriptorTable.pDescriptorRanges = &range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = 1;
    rs_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rs_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rs_params[1].Constants.Num32BitValues = 3;
    create_root_signature(context.device, &rs_desc, &context.root_signature);
    context.pipeline_state = create_compute_pipeline_state(context.device, context.root_signature, use_dxil ? cs_dxil : cs_dxbc);

    heap = create_gpu_descriptor_heap(context.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    resource = create_default_buffer(context.device, 64,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    memset(&uav_desc, 0, sizeof(uav_desc));
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    gpu_h = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(heap);
    uav_desc.Buffer.NumElements = 2;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.StructureByteStride = 16;
    uav_desc.Buffer.CounterOffsetInBytes = 0;

    ID3D12Device_CreateUnorderedAccessView(context.device, resource, NULL, &uav_desc, gpu_h);

    ID3D12GraphicsCommandList_SetDescriptorHeaps(context.list, 1, &heap);
    ID3D12GraphicsCommandList_SetComputeRootSignature(context.list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(context.list, context.pipeline_state);
    ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(context.list, 0,
        ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(heap));

    for (i = 0; i < 4; i++)
    {
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 1, 0);
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, i, 1);
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 0xf001 + i, 2);
        ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);
    }

    /* This is undefined. Accesses a structure out of bounds. */
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 1, 0);
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 4, 1);
    ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 0xbaad, 2);
    ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);

    uav_barrier(context.list, NULL);

    /* This is definitely OOB. */
    for (i = 0; i < 5; i++)
    {
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 2, 0);
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, i, 1);
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstant(context.list, 1, 0xbad, 2);
        ID3D12GraphicsCommandList_Dispatch(context.list, 1, 1, 1);
    }

    transition_resource_state(context.list, resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    get_buffer_readback_with_command_list(resource, DXGI_FORMAT_R32_UINT, &rb, context.queue, context.list);

    for (i = 0; i < 9; i++)
    {
        uint32_t v = get_readback_uint(&rb, 4 + i, 0, 0);

        if (i < 4)
            ok(v == 0xf001 + i, "Index %u: Expected #%x, got #%x.\n", 0xf001 + i, v);
        else if (i == 4)
        {
            /* UB, either 0 or 0xbaad happens depending how driver deals with robustness. */
            ok(v == 0 || v == 0xbaad, "UB scenario: got #%x, expected 0 or 0xbaad.\n", v);
            if (v == 0xbaad)
                skip("Driver checks robustness based on structure element alone.\n");
            else
                skip("Driver checks robustness based on byte offset.\n");
        }
        else
            ok(v == 0, "OOB scenario: got #%x, expected 0.\n", v);
    }

    release_resource_readback(&rb);
    ID3D12DescriptorHeap_Release(heap);
    ID3D12Resource_Release(resource);
    destroy_test_context(&context);
}

void test_uav_robustness_oob_structure_element_dxbc(void)
{
    test_uav_robustness_oob_structure_element(false);
}

void test_uav_robustness_oob_structure_element_dxil(void)
{
    test_uav_robustness_oob_structure_element(true);
}
