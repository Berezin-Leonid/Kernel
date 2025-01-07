/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

uintptr_t
get_phys_addr(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P))
        return -1;
    if (!(uvpdp[VPDP(va)] & PTE_P))
        return -1;
    if (uvpdp[VPDP(va)] & PTE_PS)
        return PTE_ADDR(uvpdp[VPDP(va)]) + ((uintptr_t)va & ((1ULL << PDP_SHIFT) - 1));
    if (!(uvpd[VPD(va)] & PTE_P))
        return -1;
    if ((uvpd[VPD(va)] & PTE_PS))
        return PTE_ADDR(uvpd[VPD(va)]) + ((uintptr_t)va & ((1ULL << PD_SHIFT) - 1));
    if (!(uvpt[VPT(va)] & PTE_P))
        return -1;
    return PTE_ADDR(uvpt[VPT(va)]) + PAGE_OFFSET(va);
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region.
     * NOTE: Skip over larger pages/page directories for efficiency */
    // LAB 11: Your code here:

    int res = 0;
    (void)fun, (void)arg;

    for (uintptr_t addr_4 = 0; addr_4 < MAX_USER_ADDRESS; addr_4 += 1ULL << PML4_SHIFT)
        if (uvpml4[VPML4(addr_4)] & PTE_P)
            for (uintptr_t addr_3 = 0; addr_3 < 1ULL << PML4_SHIFT; addr_3 += 1ULL << PDP_SHIFT)
                if (uvpdp[VPDP(addr_4 + addr_3)] & PTE_P)
                    for (uintptr_t addr_2 = 0; addr_2 < 1ULL << PDP_SHIFT; addr_2 += 1ULL << PD_SHIFT)
                        if (uvpd[VPD(addr_4 + addr_3 + addr_2)] & PTE_P)
                            for (uintptr_t addr_1 = 0; addr_1 < 1ULL << PD_SHIFT; addr_1 += 1ULL << PT_SHIFT)
                                if (uvpt[VPT(addr_4 + addr_3 + addr_2 + addr_1)] & PTE_P && uvpt[VPT(addr_4 + addr_3 + addr_2 + addr_1)] & PTE_SHARE)
                                    res = fun((void *)(addr_4 + addr_3 + addr_2 + addr_1), (void *)(addr_4 + addr_3 + addr_2 + addr_1 + PAGE_SIZE), arg);


    return res;
}
