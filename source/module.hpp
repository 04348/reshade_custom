#pragma once

#include <d3d9.h>
#include <d3d11.h>

//struct __declspec(uuid("F1006E9A-1C51-4AF4-ACEF-3605D2D4C8EE")) Direct3DDevice9;
//struct __declspec(uuid("BC52FCE4-1EAC-40C8-84CF-863600BBAA01")) Direct3DSwapChain9;

using draw_fx_func_9 = void(void);
using draw_fx_func_10 = void(void);
//using draw_fx_func_11 = void(void*);
typedef void(*draw_fx_func_11)(void*);

class IModule {
public:
	virtual ~IModule() = default;

	virtual void on_Present_9() = 0;
	virtual HRESULT CreateVertexShader_9(const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) = 0;
	virtual HRESULT SetVertexShader_9(IDirect3DVertexShader9 *pShader) = 0;
	virtual HRESULT CreatePixelShader_9(const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) = 0;
	virtual HRESULT SetPixelShader_9(IDirect3DPixelShader9 *pShader) = 0;

	virtual void on_Present_10() = 0;
	virtual HRESULT CreateVertexShader_10(const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) = 0;
	virtual HRESULT SetVertexShader_10(IDirect3DVertexShader9 *pShader) = 0;
	virtual HRESULT CreatePixelShader_10(const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) = 0;
	virtual HRESULT SetPixelShader_10(IDirect3DPixelShader9 *pShader) = 0;

	virtual void on_Present_11(void* runtime) = 0;
	virtual HRESULT CreateVertexShader_11(ID3D11Device* orig, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader) = 0;
	virtual void SetVertexShader_11(ID3D11DeviceContext* orig, ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
	virtual HRESULT CreatePixelShader_11(ID3D11Device* orig, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader) = 0;
	virtual void SetPixelShader_11(ID3D11DeviceContext* orig, ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) = 0;
	virtual void RegisterDrawFxFunc(draw_fx_func_11) = 0;
};

typedef IModule*(*CreateModuleType)();
