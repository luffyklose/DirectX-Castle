//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer cbPerObject
// {
//
//   float4x4 gWorld;                   // Offset:    0 Size:    64
//   float4x4 gTexTransform;            // Offset:   64 Size:    64
//
// }
//
// cbuffer cbPass
// {
//
//   float4x4 gView;                    // Offset:    0 Size:    64 [unused]
//   float4x4 gInvView;                 // Offset:   64 Size:    64 [unused]
//   float4x4 gProj;                    // Offset:  128 Size:    64 [unused]
//   float4x4 gInvProj;                 // Offset:  192 Size:    64 [unused]
//   float4x4 gViewProj;                // Offset:  256 Size:    64
//   float4x4 gInvViewProj;             // Offset:  320 Size:    64 [unused]
//   float3 gEyePosW;                   // Offset:  384 Size:    12 [unused]
//   float cbPerObjectPad1;             // Offset:  396 Size:     4 [unused]
//   float2 gRenderTargetSize;          // Offset:  400 Size:     8 [unused]
//   float2 gInvRenderTargetSize;       // Offset:  408 Size:     8 [unused]
//   float gNearZ;                      // Offset:  416 Size:     4 [unused]
//   float gFarZ;                       // Offset:  420 Size:     4 [unused]
//   float gTotalTime;                  // Offset:  424 Size:     4 [unused]
//   float gDeltaTime;                  // Offset:  428 Size:     4 [unused]
//   float4 gAmbientLight;              // Offset:  432 Size:    16 [unused]
//   
//   struct Light
//   {
//       
//       float3 Strength;               // Offset:  448
//       float FalloffStart;            // Offset:  460
//       float3 Direction;              // Offset:  464
//       float FalloffEnd;              // Offset:  476
//       float3 Position;               // Offset:  480
//       float SpotPower;               // Offset:  492
//
//   } gLights[16];                     // Offset:  448 Size:   768 [unused]
//
// }
//
// cbuffer cbMaterial
// {
//
//   float4 gDiffuseAlbedo;             // Offset:    0 Size:    16 [unused]
//   float3 gFresnelR0;                 // Offset:   16 Size:    12 [unused]
//   float gRoughness;                  // Offset:   28 Size:     4 [unused]
//   float4x4 gMatTransform;            // Offset:   32 Size:    64
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// cbPerObject                       cbuffer      NA          NA            cb0      1 
// cbPass                            cbuffer      NA          NA            cb1      1 
// cbMaterial                        cbuffer      NA          NA            cb2      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// POSITION                 0   xyz         0     NONE   float   xyz 
// NORMAL                   0   xyz         1     NONE   float   xyz 
// TEXCOORD                 0   xy          2     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float   xyzw
// POSITION                 0   xyz         1     NONE   float   xyz 
// NORMAL                   0   xyz         2     NONE   float   xyz 
// TEXCOORD                 0   xy          3     NONE   float   xy  
//
vs_5_0
dcl_globalFlags refactoringAllowed
dcl_constantbuffer CB0[8], immediateIndexed
dcl_constantbuffer CB1[20], immediateIndexed
dcl_constantbuffer CB2[4], immediateIndexed
dcl_input v0.xyz
dcl_input v1.xyz
dcl_input v2.xy
dcl_output_siv o0.xyzw, position
dcl_output o1.xyz
dcl_output o2.xyz
dcl_output o3.xy
dcl_temps 2
mov r0.xyz, v0.xyzx
mov r0.w, l(1.000000)
dp4 r1.w, r0.xyzw, cb0[3].xyzw
dp4 r1.x, r0.xyzw, cb0[0].xyzw
dp4 r1.y, r0.xyzw, cb0[1].xyzw
dp4 r1.z, r0.xyzw, cb0[2].xyzw
dp4 o0.x, r1.xyzw, cb1[16].xyzw
dp4 o0.y, r1.xyzw, cb1[17].xyzw
dp4 o0.z, r1.xyzw, cb1[18].xyzw
dp4 o0.w, r1.xyzw, cb1[19].xyzw
mov o1.xyz, r1.xyzx
dp3 o2.x, v1.xyzx, cb0[0].xyzx
dp3 o2.y, v1.xyzx, cb0[1].xyzx
dp3 o2.z, v1.xyzx, cb0[2].xyzx
mov r0.xy, v2.xyxx
mov r0.z, l(1.000000)
dp3 r1.x, r0.xyzx, cb0[4].xywx
dp3 r1.y, r0.xyzx, cb0[5].xywx
dp3 r1.z, r0.xyzx, cb0[6].xywx
dp3 r1.w, r0.xyzx, cb0[7].xywx
dp4 o3.x, r1.xyzw, cb2[2].xyzw
dp4 o3.y, r1.xyzw, cb2[3].xyzw
ret 
// Approximately 23 instruction slots used