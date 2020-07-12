# Lab 2: Memory Management

## Exercise 1

`init_buddy` 中对每个 page 调用 `buddy_free_pages` 来构造最初的空闲链表，因此首先实现 `buddy_free_pages`。一开始每个 page 的 order 都设置成 0，意味着每个 page 单独构成一个独立的块，随着 `buddy_free_pages` 的调用，空闲的块应该不断地合并，最终初始化结束后，应该只有最高 order（9）的空闲链表里面有空闲块。所以 `buddy_free_pages` 里面需要不断地找到当前块的 buddy，如果它没有被分配（`PG_buddy` 是 1），也没有被分割（order 与当前块一致），则合并。合并的过程中需要将原先两个小的 buddy 移出它们所在的空闲链表，但不需要修改原先两个小的 buddy 的 `PG_buddy` 位和 order 值，因为每个块只有一个对应的 buddy，合并后只有 `split` 才能再分开它们，那时候会恰当地设置这两个值，此处无需关心。一层一层合并，直到最高 order 或者无法继续合并，通过 `set_page_order_buddy` 设置最终得到的合并后的 page 块，最后添加到对应 order 的空闲链表中。

`buddy_get_pages` 只需做一个边界检查，然后简单地调用 `__alloc_page` 即可。后者进行真正的分配动作，从所需分配的 order 对应的空闲链表开始往上寻找，直到某个空闲链表非空，随便取出里面的一个空闲块，如果 order 正好，则分配完成；如果 order 大于需要的大小，则调用 `split` 来分割，这里修改了一下 `split`，在最后给分割得到的最终正确 order 的块设置了 `PG_buddy` 位和 order 值，以保持它的一致性。`split` 结束后得到的最后一个块，就是最终分配的块。在 `__alloc_page` 返回前，清除掉所分配的块的 `PG_buddy` 位，以避免此后被错误地合并。由于在 `buddy_free_pages` 里需要利用已分配的块的 order 值，所以这里分配的时候需要保证 order 值的正确性。

分配块时需要从空闲链表的 node 反向得到 page，借鉴了 uCore OS 的方式，写了个 `ln2page` 宏来通过 struct 字段的偏移量计算 page 指针。

最终经过调试修改，通过了 `test_buddy` 测试。

## Exercise 2

`query_in_pgtbl` 一级一级地使用 `get_next_ptp` 查找下一级页表地址，如果下一级页表不存在则返回 `-ENOMAPPING`，如果某一级的页表项不是指向一个页表，而是指向一个大页（`res == BLOCK_PTP`），则直接返回其对应的物理地址，否则最终经过 4 级查询，可得到 L3 的页表项，存有一个页的物理地址。

`map_range_in_pgtbl` 对要映射的每个页执行类似 `query_in_pgtbl` 的查询，区别是调用 `get_next_ptp` 时让它在不存在页表时创建新页表，并且在最后一级（L3）处不应使用 `get_next_ptp` 获取下一级页表，而是直接设置 L3 页表项的值，填入要映射的物理地址，再配置一些属性。

`unmap_range_in_pgtbl` 与 `map_range_in_pgtbl` 相似，区别是当页表不存在时不新建，以及查询结束后清空页表项而不是设置它。

## Exercise 3

在 `map_kernel_space` 中获得当前 PGD 地址，然后参考 `boot/mmu.c` 中初始化页表的部分，获得 128MB 到 256MB 的区域所对应的 PMD，并设置各 2MB 页的映射即可。

## Challenge 1

TODO。

## Challenge 2

修改 `map_range_in_pgtbl`，根据 `va` 的对齐情况，优先映射 1G 页，其次 2M 页，最后 4K 页。由于是根据 `va` 和 `len` 的大小情况来构造映射的，在 unmap 的时候应该使用相同的 `va` 和 `len`，否则可能出现问题，例如 `va` 在一个 1G 页内，但是 `len` 不是 1G，那么 unmap 就会出错。
