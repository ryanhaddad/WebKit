/*
 * Copyright (C) 2015-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JITBitAndGenerator.h"

#if ENABLE(JIT)

namespace JSC {

void JITBitAndGenerator::generateFastPath(CCallHelpers& jit)
{
#if USE(JSVALUE64)
    ASSERT(m_scratchGPR != InvalidGPRReg);
    ASSERT(m_scratchGPR != m_left.payloadGPR());
    ASSERT(m_scratchGPR != m_right.payloadGPR());
#else
    UNUSED_PARAM(m_scratchGPR);
#endif

    ASSERT(!m_leftOperand.isConstInt32() || !m_rightOperand.isConstInt32());

    m_didEmitFastPath = true;

    if (m_leftOperand.isConstInt32() || m_rightOperand.isConstInt32()) {
        JSValueRegs var = m_leftOperand.isConstInt32() ? m_right : m_left;
        SnippetOperand& constOpr = m_leftOperand.isConstInt32() ? m_leftOperand : m_rightOperand;
        
        // Try to do intVar & intConstant.
        m_slowPathJumpList.append(jit.branchIfNotInt32(var));
        
        if (constOpr.asConstInt32() != static_cast<int32_t>(0xffffffff)) {
#if USE(JSVALUE64)
            jit.and64(CCallHelpers::Imm32(constOpr.asConstInt32()), var.payloadGPR(), m_result.payloadGPR());
            if (constOpr.asConstInt32() >= 0)
                jit.boxInt32(m_result.payloadGPR(), m_result);
#else
            jit.moveValueRegs(var, m_result);
            jit.and32(CCallHelpers::Imm32(constOpr.asConstInt32()), m_result.payloadGPR());
#endif
        } else
            jit.moveValueRegs(var, m_result);
        return;
    }

#if USE(JSVALUE64)
    if (m_leftOperand.definitelyIsBoolean() && m_rightOperand.definitelyIsBoolean()) {
        jit.and32(m_left.payloadGPR(), m_right.payloadGPR(), m_result.payloadGPR());
        jit.and32(CCallHelpers::TrustedImm32(1), m_result.payloadGPR());
        jit.boxInt32(m_result.payloadGPR(), m_result);
        return;
    }
#endif

    ASSERT(!m_leftOperand.isConstInt32() && !m_rightOperand.isConstInt32());

    // Try to do intVar & intVar.
#if USE(JSVALUE64)
    jit.and64(m_left.payloadGPR(), m_right.payloadGPR(), m_scratchGPR);
    m_slowPathJumpList.append(jit.branchIfNotInt32(m_scratchGPR));
    jit.move(m_scratchGPR, m_result.payloadGPR());
#else
    m_slowPathJumpList.append(jit.branchIfNotInt32(m_left));
    m_slowPathJumpList.append(jit.branchIfNotInt32(m_right));
    jit.moveValueRegs(m_left, m_result);
    jit.and32(m_right.payloadGPR(), m_result.payloadGPR());
#endif
}

} // namespace JSC

#endif // ENABLE(JIT)
