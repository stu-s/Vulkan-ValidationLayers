#include "xglrenderframework.h"

XglDescriptorSetObj::XglDescriptorSetObj(XglDevice *device)
{
    m_device = device;
    m_nextSlot = 0;

}

void XglDescriptorSetObj::AttachMemoryView(XglConstantBufferObj *constantBuffer)
{
    m_memoryViews.push_back(&constantBuffer->m_constantBufferView);
    m_memorySlots.push_back(m_nextSlot);
    m_nextSlot++;

}
void XglDescriptorSetObj::AttachSampler(XglSamplerObj *sampler)
{
    m_samplers.push_back(&sampler->m_sampler);
    m_samplerSlots.push_back(m_nextSlot);
    m_nextSlot++;

}
void XglDescriptorSetObj::AttachImageView(XglTextureObj *texture)
{
    m_imageViews.push_back(&texture->m_textureViewInfo);
    m_imageSlots.push_back(m_nextSlot);
    m_nextSlot++;

}
XGL_DESCRIPTOR_SLOT_INFO* XglDescriptorSetObj::GetSlotInfo(vector<int>slots,
                                                           vector<XGL_DESCRIPTOR_SET_SLOT_TYPE>types,
                                                           vector<XGL_OBJECT>objs )
{
    int nSlots = m_memorySlots.size() + m_imageSlots.size() + m_samplerSlots.size();
    m_slotInfo = (XGL_DESCRIPTOR_SLOT_INFO*) malloc( nSlots * sizeof(XGL_DESCRIPTOR_SLOT_INFO) );
    memset(m_slotInfo,0,nSlots*sizeof(XGL_DESCRIPTOR_SLOT_INFO));

    for (int i=0; i<nSlots; i++)
    {
        m_slotInfo[i].slotObjectType = XGL_SLOT_UNUSED;
    }

    for (int i=0; i<slots.size(); i++)
    {
        for (int j=0; j<m_memorySlots.size(); j++)
        {
            if ( (XGL_OBJECT) m_memoryViews[j] == objs[i])
            {
                m_slotInfo[m_memorySlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_memorySlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_imageSlots.size(); j++)
        {
            if ( (XGL_OBJECT) m_imageViews[j] == objs[i])
            {
                m_slotInfo[m_imageSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_imageSlots[j]].slotObjectType = types[i];
            }
        }
        for (int j=0; j<m_samplerSlots.size(); j++)
        {
            if ( (XGL_OBJECT) m_samplers[j] == objs[i])
            {
                m_slotInfo[m_samplerSlots[j]].shaderEntityIndex = slots[i];
                m_slotInfo[m_samplerSlots[j]].slotObjectType = types[i];
            }
        }
    }

    for (int i=0;i<nSlots;i++)
    {
        printf("SlotInfo[%d]:  Index = %d, Type = %d\n",i,m_slotInfo[i].shaderEntityIndex, m_slotInfo[i].slotObjectType);
        fflush(stdout);
    }

    return(m_slotInfo);

}

void XglDescriptorSetObj::BindCommandBuffer(XGL_CMD_BUFFER commandBuffer)
{
    XGL_RESULT err;

    // Create descriptor set for a uniform resource
    memset(&m_descriptorInfo,0,sizeof(m_descriptorInfo));
    m_descriptorInfo.sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO;
    m_descriptorInfo.slots = m_nextSlot;

    // Create a descriptor set with requested number of slots
    err = xglCreateDescriptorSet( m_device->device(), &m_descriptorInfo, &m_rsrcDescSet );

    // Bind memory to the descriptor set
    err = m_device->AllocAndBindGpuMemory(m_rsrcDescSet, "DescriptorSet", &m_descriptor_set_mem);

    xglBeginDescriptorSetUpdate( m_rsrcDescSet );
    xglClearDescriptorSetSlots(m_rsrcDescSet, 0, m_nextSlot);
    for (int i=0; i<m_memoryViews.size();i++)
    {
        xglAttachMemoryViewDescriptors( m_rsrcDescSet, m_memorySlots[i], 1, m_memoryViews[i] );
    }
    for (int i=0; i<m_samplers.size();i++)
    {
        xglAttachSamplerDescriptors( m_rsrcDescSet, m_samplerSlots[i], 1, m_samplers[i] );
    }
    for (int i=0; i<m_imageViews.size();i++)
    {
        xglAttachImageViewDescriptors( m_rsrcDescSet, m_imageSlots[i], 1, m_imageViews[i] );
    }
    xglEndDescriptorSetUpdate( m_rsrcDescSet );

    // bind pipeline, vertex buffer (descriptor set) and WVP (dynamic memory view)
    xglCmdBindDescriptorSet(commandBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, 0, m_rsrcDescSet, 0 );
}


XglTextureObj::XglTextureObj(XglDevice *device):
    m_texture(XGL_NULL_HANDLE),
    m_textureMem(XGL_NULL_HANDLE),
    m_textureView(XGL_NULL_HANDLE)
{
    m_device = device;
    const XGL_FORMAT tex_format = { XGL_CH_FMT_B8G8R8A8, XGL_NUM_FMT_UNORM };
    const XGL_INT tex_width = 16;
    const XGL_INT tex_height = 16;
    const uint32_t tex_colors[2] = { 0xffff0000, 0xff00ff00 };
    XGL_RESULT err;
    XGL_UINT i;

    memset(&m_textureViewInfo,0,sizeof(m_textureViewInfo));

    m_textureViewInfo.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;

    const XGL_IMAGE_CREATE_INFO image = {
        .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = XGL_IMAGE_2D,
        .format = tex_format,
        .extent = { tex_width, tex_height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = XGL_LINEAR_TILING,
        .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
        .flags = 0,
    };

    XGL_MEMORY_ALLOC_INFO mem_alloc;
        mem_alloc.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = 0;
        mem_alloc.alignment = 0;
        mem_alloc.flags = 0;
        mem_alloc.heapCount = 0;
        mem_alloc.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    XGL_IMAGE_VIEW_CREATE_INFO view;
        view.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.image = XGL_NULL_HANDLE;
        view.viewType = XGL_IMAGE_VIEW_2D;
        view.format = image.format;
        view.channels.r = XGL_CHANNEL_SWIZZLE_R;
        view.channels.g = XGL_CHANNEL_SWIZZLE_G;
        view.channels.b = XGL_CHANNEL_SWIZZLE_B;
        view.channels.a = XGL_CHANNEL_SWIZZLE_A;
        view.subresourceRange.aspect = XGL_IMAGE_ASPECT_COLOR;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.mipLevels = 1;
        view.subresourceRange.baseArraySlice = 0;
        view.subresourceRange.arraySize = 1;
        view.minLod = 0.0f;

    XGL_MEMORY_REQUIREMENTS mem_reqs;
    XGL_SIZE mem_reqs_size;

    /* create image */
    err = xglCreateImage(m_device->device(), &image, &m_texture);
    assert(!err);

    err = xglGetObjectInfo(m_texture,
            XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, &mem_reqs);
    assert(!err && mem_reqs_size == sizeof(mem_reqs));

    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.alignment = mem_reqs.alignment;
    mem_alloc.heapCount = mem_reqs.heapCount;
    memcpy(mem_alloc.heaps, mem_reqs.heaps,
            sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

    /* allocate memory */
    err = xglAllocMemory(m_device->device(), &mem_alloc, &m_textureMem);
    assert(!err);

    /* bind memory */
    err = xglBindObjectMemory(m_texture, m_textureMem, 0);
    assert(!err);

    /* create image view */
    view.image = m_texture;
    err = xglCreateImageView(m_device->device(), &view, &m_textureView);
    assert(!err);


    const XGL_IMAGE_SUBRESOURCE subres = {
        .aspect = XGL_IMAGE_ASPECT_COLOR,
        .mipLevel = 0,
        .arraySlice = 0,
    };
    XGL_SUBRESOURCE_LAYOUT layout;
    XGL_SIZE layout_size;
    XGL_VOID *data;
    XGL_INT x, y;

    err = xglGetImageSubresourceInfo(m_texture, &subres,
            XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
    assert(!err && layout_size == sizeof(layout));

    err = xglMapMemory(m_textureMem, 0, &data);
    assert(!err);

    for (y = 0; y < tex_height; y++) {
        uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
        for (x = 0; x < tex_width; x++)
            row[x] = tex_colors[(x & 1) ^ (y & 1)];
    }

    err = xglUnmapMemory(m_textureMem);
    assert(!err);

    m_textureViewInfo.view = m_textureView;

}



XglSamplerObj::XglSamplerObj(XglDevice *device)
{
    XGL_RESULT err = XGL_SUCCESS;

    m_device = device;
    memset(&m_samplerCreateInfo,0,sizeof(m_samplerCreateInfo));
    m_samplerCreateInfo.sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    m_samplerCreateInfo.magFilter = XGL_TEX_FILTER_NEAREST;
    m_samplerCreateInfo.minFilter = XGL_TEX_FILTER_NEAREST;
    m_samplerCreateInfo.mipMode = XGL_TEX_MIPMAP_BASE;
    m_samplerCreateInfo.addressU = XGL_TEX_ADDRESS_WRAP;
    m_samplerCreateInfo.addressV = XGL_TEX_ADDRESS_WRAP;
    m_samplerCreateInfo.addressW = XGL_TEX_ADDRESS_WRAP;
    m_samplerCreateInfo.mipLodBias = 0.0;
    m_samplerCreateInfo.maxAnisotropy = 0.0;
    m_samplerCreateInfo.compareFunc = XGL_COMPARE_NEVER;
    m_samplerCreateInfo.minLod = 0.0;
    m_samplerCreateInfo.maxLod = 0.0;
    m_samplerCreateInfo.borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE;

    err = xglCreateSampler(m_device->device(),&m_samplerCreateInfo, &m_sampler);

}

XglConstantBufferObj::XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data)
{
    XGL_RESULT err = XGL_SUCCESS;
    XGL_UINT8 *pData;
    XGL_MEMORY_ALLOC_INFO           alloc_info = {};
    m_device = device;
    m_numVertices = constantCount;
    m_stride = constantSize;

    memset(&m_constantBufferView,0,sizeof(m_constantBufferView));
    memset(&m_constantBufferMem,0,sizeof(m_constantBufferMem));

    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = constantCount * constantSize;
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(m_device->device(), &alloc_info, &m_constantBufferMem);

    err = xglMapMemory(m_constantBufferMem, 0, (XGL_VOID **) &pData);

    memcpy(pData, data, alloc_info.allocationSize);

    err = xglUnmapMemory(m_constantBufferMem);

    // set up the memory view for the constant buffer
    this->m_constantBufferView.stride = 16;
    this->m_constantBufferView.range  = alloc_info.allocationSize;
    this->m_constantBufferView.offset = 0;
    this->m_constantBufferView.mem    = m_constantBufferMem;
    this->m_constantBufferView.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    this->m_constantBufferView.format.numericFormat = XGL_NUM_FMT_FLOAT;
    this->m_constantBufferView.state  = XGL_MEMORY_STATE_DATA_TRANSFER;
}

void XglConstantBufferObj::SetMemoryState(XGL_CMD_BUFFER cmdBuffer, XGL_MEMORY_STATE newState)
{
    if (this->m_constantBufferView.state == newState)
        return;

    // open the command buffer
    XGL_RESULT err = xglBeginCommandBuffer( cmdBuffer, 0 );
    ASSERT_XGL_SUCCESS(err);

    XGL_MEMORY_STATE_TRANSITION transition = {};
    transition.mem = m_constantBufferMem;
    transition.oldState = XGL_MEMORY_STATE_DATA_TRANSFER;
    transition.newState = newState;
    transition.offset = 0;
    transition.regionSize = m_numVertices * m_stride;

    // write transition to the command buffer
    xglCmdPrepareMemoryRegions( cmdBuffer, 1, &transition );
    this->m_constantBufferView.state = newState;

    // finish recording the command buffer
    err = xglEndCommandBuffer( cmdBuffer );
    ASSERT_XGL_SUCCESS(err);

    XGL_UINT32     numMemRefs=1;
    XGL_MEMORY_REF memRefs;
    // this command buffer only uses the vertex buffer memory
    memRefs.flags = 0;
    memRefs.mem = m_constantBufferMem;

    // submit the command buffer to the universal queue
    err = xglQueueSubmit( m_device->m_queue, 1, &cmdBuffer, numMemRefs, &memRefs, NULL );
    ASSERT_XGL_SUCCESS(err);
}


XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* XglShaderObj::GetStageCreateInfo(XglDescriptorSetObj descriptorSet)
{
    XGL_DESCRIPTOR_SLOT_INFO *slotInfo;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO *stageInfo = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) calloc( 1,sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) );
    stageInfo->sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo->shader.stage = m_stage;
    stageInfo->shader.shader = m_shader;
    stageInfo->shader.descriptorSetMapping[0].descriptorCount = 0;
    stageInfo->shader.linkConstBufferCount = 0;
    stageInfo->shader.pLinkConstBufferInfo = XGL_NULL_HANDLE;
    stageInfo->shader.dynamicMemoryViewMapping.slotObjectType = XGL_SLOT_UNUSED;
    stageInfo->shader.dynamicMemoryViewMapping.shaderEntityIndex = 0;

    stageInfo->shader.descriptorSetMapping[0].descriptorCount = m_memSlots.size() + m_imageSlots.size() + m_samplerSlots.size();
    if (stageInfo->shader.descriptorSetMapping[0].descriptorCount)
    {
        vector<int> allSlots;
        vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> allTypes;
        vector<XGL_OBJECT> allObjs;

        allSlots.reserve(m_memSlots.size() + m_imageSlots.size() + m_samplerSlots.size());
        allTypes.reserve(m_memTypes.size() + m_imageTypes.size() + m_samplerTypes.size());
        allObjs.reserve(m_memObjs.size() + m_imageObjs.size() + m_samplerObjs.size());

        if (m_memSlots.size())
        {
            allSlots.insert(allSlots.end(), m_memSlots.begin(), m_memSlots.end());
            allTypes.insert(allTypes.end(), m_memTypes.begin(), m_memTypes.end());
            allObjs.insert(allObjs.end(), m_memObjs.begin(), m_memObjs.end());
        }
        if (m_imageSlots.size())
        {
            allSlots.insert(allSlots.end(), m_imageSlots.begin(), m_imageSlots.end());
            allTypes.insert(allTypes.end(), m_imageTypes.begin(), m_imageTypes.end());
            allObjs.insert(allObjs.end(), m_imageObjs.begin(), m_imageObjs.end());
        }
        if (m_samplerSlots.size())
        {
            allSlots.insert(allSlots.end(), m_samplerSlots.begin(), m_samplerSlots.end());
            allTypes.insert(allTypes.end(), m_samplerTypes.begin(), m_samplerTypes.end());
            allObjs.insert(allObjs.end(), m_samplerObjs.begin(), m_samplerObjs.end());
        }

         slotInfo = descriptorSet.GetSlotInfo(allSlots, allTypes, allObjs);
         stageInfo->shader.descriptorSetMapping[0].pDescriptorInfo = (const XGL_DESCRIPTOR_SLOT_INFO*) slotInfo;
    }
    return stageInfo;
}

void XglShaderObj::BindShaderEntitySlotToMemory(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglConstantBufferObj *constantBuffer)
{
    m_memSlots.push_back(slot);
    m_memTypes.push_back(type);
    m_memObjs.push_back((XGL_OBJECT) &constantBuffer->m_constantBufferView);

}
void XglShaderObj::BindShaderEntitySlotToImage(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglTextureObj *texture)
{
    m_imageSlots.push_back(slot);
    m_imageTypes.push_back(type);
    m_imageObjs.push_back((XGL_OBJECT) &texture->m_textureViewInfo);

}
void XglShaderObj::BindShaderEntitySlotToSampler(int slot, XglSamplerObj *sampler)
{
    m_samplerSlots.push_back(slot);
    m_samplerTypes.push_back(XGL_SLOT_SHADER_SAMPLER);
    m_samplerObjs.push_back(sampler->m_sampler);

}
XglShaderObj::XglShaderObj(XglDevice *device, const char * shader_code, XGL_PIPELINE_SHADER_STAGE stage)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;

    m_stage = stage;
    m_device = device;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    shader_len = strlen(shader_code);
    createInfo.codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = stage;
    memcpy(((uint32_t *) createInfo.pCode + 3), shader_code, shader_len + 1);

    err = xglCreateShader(m_device->device(), &createInfo, &m_shader);
    if (err) {
        free((void *) createInfo.pCode);
    }
}
#if 0
void XglShaderObj::CreateShaderBIL(XGL_PIPELINE_SHADER_STAGE stage,
                                                  const char *shader_code,
                                                  XGL_SHADER *pshader)
{
    XGL_RESULT err = XGL_SUCCESS;
    std::vector<unsigned int> bil;
    XGL_SHADER_CREATE_INFO createInfo;
    size_t shader_len;
    XGL_SHADER shader;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    // Use Reference GLSL to BIL compiler
    GLSLtoBIL(stage, shader_code, bil);
    createInfo.pCode = bil.data();
    createInfo.codeSize = bil.size() * sizeof(unsigned int);
    createInfo.flags = 0;
    err = xglCreateShader(device(), &createInfo, &shader);

    ASSERT_XGL_SUCCESS(err);

    *pshader = shader;
}
#endif

XglPipelineObj::XglPipelineObj(XglDevice *device)
{
    XGL_RESULT err;

    m_device = device;
    m_vi_state.attributeCount = m_vi_state.bindingCount = 0;
    m_vertexBufferCount = 0;

    m_ia_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    m_ia_state.pNext = XGL_NULL_HANDLE;
    m_ia_state.topology = XGL_TOPOLOGY_TRIANGLE_LIST;
    m_ia_state.disableVertexReuse = XGL_FALSE;
    m_ia_state.provokingVertex = XGL_PROVOKING_VERTEX_LAST;
    m_ia_state.primitiveRestartEnable = XGL_FALSE;
    m_ia_state.primitiveRestartIndex = 0;

    m_rs_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    m_rs_state.pNext = &m_ia_state;
    m_rs_state.depthClipEnable = XGL_FALSE;
    m_rs_state.rasterizerDiscardEnable = XGL_FALSE;
    m_rs_state.pointSize = 1.0;

    m_render_target_format.channelFormat = XGL_CH_FMT_R8G8B8A8;
    m_render_target_format.numericFormat = XGL_NUM_FMT_UNORM;

    memset(&m_cb_state,0,sizeof(m_cb_state));
    m_cb_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    m_cb_state.pNext = &m_rs_state;
    m_cb_state.alphaToCoverageEnable = XGL_FALSE;
    m_cb_state.dualSourceBlendEnable = XGL_FALSE;
    m_cb_state.logicOp = XGL_LOGIC_OP_COPY;

    m_cb_attachment_state.blendEnable = XGL_FALSE;
    m_cb_attachment_state.format = m_render_target_format;
    m_cb_attachment_state.channelWriteMask = 0xF;
    m_cb_state.attachment[0] = m_cb_attachment_state;

    m_db_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO,
    m_db_state.pNext = &m_cb_state,
    m_db_state.format.channelFormat = XGL_CH_FMT_R32;
    m_db_state.format.numericFormat = XGL_NUM_FMT_DS;


};

void XglPipelineObj::AddShader(XglShaderObj* shader)
{
    m_shaderObjs.push_back(shader);
}

void XglPipelineObj::AddVertexInputAttribs(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count)
{
    m_vi_state.pVertexAttributeDescriptions = vi_attrib;
    m_vi_state.attributeCount = count;
}

void XglPipelineObj::AddVertexInputBindings(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count)
{
    m_vi_state.pVertexBindingDescriptions = vi_binding;
    m_vi_state.bindingCount = count;
}

void XglPipelineObj::AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding)
{
    m_vertexBufferObjs.push_back(vertexDataBuffer);
    m_vertexBufferBindings.push_back(binding);
    m_vertexBufferCount++;
}

void XglPipelineObj::BindPipelineCommandBuffer(XGL_CMD_BUFFER m_cmdBuffer, XglDescriptorSetObj descriptorSet)
{
    XGL_RESULT err;
    XGL_VOID* head_ptr = &m_db_state;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO info = {};

    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shaderCreateInfo;
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vertexInputAttrib;

    for (int i=0; i<m_shaderObjs.size(); i++)
    {
        shaderCreateInfo = m_shaderObjs[i]->GetStageCreateInfo(descriptorSet);
        shaderCreateInfo->pNext = head_ptr;
        head_ptr = shaderCreateInfo;
    }

    if (m_vi_state.attributeCount && m_vi_state.bindingCount)
    {
        m_vi_state.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
        m_vi_state.pNext = head_ptr;
        head_ptr = &m_vi_state;
    }

    info.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = head_ptr;
    info.flags = 0;

    err = xglCreateGraphicsPipeline(m_device->device(), &info, &m_pipeline);

    err = m_device->AllocAndBindGpuMemory(m_pipeline, "Pipeline", &m_pipe_mem);

    xglCmdBindPipeline( m_cmdBuffer, XGL_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );


    for (int i=0; i < m_vertexBufferCount; i++)
    {
        xglCmdBindVertexData(m_cmdBuffer, m_vertexBufferObjs[i]->m_constantBufferView.mem, m_vertexBufferObjs[i]->m_constantBufferView.offset, m_vertexBufferBindings[i]);
    }

}


XglMemoryRefManager::XglMemoryRefManager() {

}
void XglMemoryRefManager::AddMemoryRef(XglConstantBufferObj *constantBuffer) {
    m_bufferObjs.push_back(&constantBuffer->m_constantBufferMem);
}
void XglMemoryRefManager::AddMemoryRef(XglTextureObj *texture) {
    m_bufferObjs.push_back(&texture->m_textureMem);
}
XGL_MEMORY_REF* XglMemoryRefManager::GetMemoryRefList() {

    XGL_MEMORY_REF *localRefs;
    XGL_UINT32     numRefs=m_bufferObjs.size();

    if (numRefs <= 0)
        return NULL;

    localRefs = (XGL_MEMORY_REF*) malloc( numRefs * sizeof(XGL_MEMORY_REF) );
    for (int i=0; i<numRefs; i++)
    {
        localRefs[i].flags = 0;
        localRefs[i].mem = *m_bufferObjs[i];
    }
    return localRefs;
}
int XglMemoryRefManager::GetNumRefs() {
    return m_bufferObjs.size();
}
