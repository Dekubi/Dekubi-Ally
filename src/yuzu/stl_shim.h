#pragma once
#include <cstdint>
extern "C" {
void* __cdecl __std_rotate(void* F, void* M, void* L) noexcept { auto* f=(uint8_t*)F; auto* m=(uint8_t*)M; auto* l=(uint8_t*)L; if(f==m) return l; if(m==l) return f; uint8_t* r=f+(l-m); uint8_t* n=m; while(f!=n){ uint8_t t=*f; *f++=*n; *n++=t; if(n==l) n=m; else if(f==m) m=n; } return r; }
void* __cdecl __std_unique_4(void* F, void* L) noexcept { auto* f=(uint32_t*)F; auto* l=(uint32_t*)L; if(f==l) return l; uint32_t* r=f; while(++f!=l){ if(*r!=*f && ++r!=f) *r=*f; } return ++r; }
void* __cdecl __std_unique_8(void* F, void* L) noexcept { auto* f=(uint64_t*)F; auto* l=(uint64_t*)L; if(f==l) return l; uint64_t* r=f; while(++f!=l){ if(*r!=*f && ++r!=f) *r=*f; } return ++r; }
void __cdecl __std_replace_copy_1(const void* F, const void* L, void* D, uint8_t O, uint8_t N) noexcept { auto* f=(const uint8_t*)F; auto* l=(const uint8_t*)L; auto* d=(uint8_t*)D; for(;f!=l;++f,++d) *d=(*f==O)?N:*f; }
void __cdecl __std_replace_copy_2(const void* F, const void* L, void* D, uint16_t O, uint16_t N) noexcept { auto* f=(const uint16_t*)F; auto* l=(const uint16_t*)L; auto* d=(uint16_t*)D; for(;f!=l;++f,++d) *d=(*f==O)?N:*f; }
struct MM2U { const void* a; const void* b; }; MM2U __cdecl __std_minmax_element_2u(const void* F, const void* L) noexcept { auto* f=(const uint16_t*)F; auto* l=(const uint16_t*)L; MM2U r{f,f}; if(f==l) return r; auto* mn=f; auto* mx=f; while(++f!=l){ if(*f<*mn) mn=f; if(*f>=*mx) mx=f; } r.a=mn; r.b=mx; return r; }
}

