#!/usr/bin/python

import sys
from operator import add
import math

def main():
    debug = 0
    inconsistencies = 0
    reserved_blocks = set([0,1])
    reserved_inodes = set([0,1,3,4,5,6,7,8,9,10]) #Inode number starts from 1; https://www.vidarholen.net/contents/junk/inodes.html, 1-10 reserved
    duplicate_block_dict = {}
    inode_linkcount_dict = {}
    inode_to_dirname_linkedInode_dict = {}
    dotdot_entry_dict = {}

    if len(sys.argv) <= 1:
        sys.stderr.write('Needs a file summary csv-file as argument\n')
        exit(1)
    try:
        file_summary = open(sys.argv[1], "r")
    except:
        sys.stderr.write('Error opening file\n')
        exit(1)

    if len(sys.argv) == 3 and sys.argv[2] == 'debug':
        debug = 1

    lines = file_summary.readlines()
    for line in lines:
        fields = line.split(",")
        output_type = fields[0]
        
        if output_type == 'SUPERBLOCK':
            blocks_count = int(fields[1])
            inode_count = int(fields[2])
            block_size = int(fields[3])
            inode_size = int(fields[4])
            reserved_blocks.add(blocks_count)
            free_blocks = [0]*(blocks_count+1)
            referenced_blocks = [0]*(blocks_count+1)
            free_inodes = [0]*(inode_count+1)
            allocated_inodes = [0]*(inode_count+1)
            inode_linkcount_counter = [0]*(inode_count+1)
            inode_par_array = [0]*(inode_count+1)
            if block_size == 1024:
                reserved_blocks.add(2)
            if debug:
                print("Blocks_count: {}, Inode_count: {}".format(blocks_count, inode_count))
        elif output_type == 'GROUP':
            reserved_blocks.add(int(fields[6]))
            reserved_blocks.add(int(fields[7]))
            reserved_blocks.add(int(fields[8]))
            blocks_for_inode = (inode_size*inode_count)/block_size
            blocks_for_inode = math.ceil(blocks_for_inode)
            if debug:
                print("Blocks for inode: {}".format(blocks_for_inode))
            blocks_for_inode -= 1
            while blocks_for_inode != 0:
                reserved_blocks.add(int(fields[8])+blocks_for_inode)
                blocks_for_inode -= 1
            if debug:
                print("Reserved blocks: {}".format(reserved_blocks))
        elif output_type == 'BFREE':
            free_blocks[int(fields[1])] = 1
        elif output_type == 'IFREE':
            free_inodes[int(fields[1])] = 1
        elif output_type == 'INODE':
            inode = int(fields[1])
            allocated_inodes[inode] += 1
            inode_linkcount_dict[inode] = int(fields[6])
            for i in range(12,27):
                block = int(fields[i])
                if block == 0:
                    continue

                if i == 24:
                    indirection = "INDIRECT BLOCK"
                    offset = 12
                elif i == 25:
                    indirection = "DOUBLE INDIRECT BLOCK"
                    offset = 12 + 256
                elif i == 26:
                    indirection = "TRIPLE INDIRECT BLOCK"
                    offset = 12 + (256 + 1) * 256
                else:
                    indirection = "BLOCK"
                    offset = i - 12

                if block < 0 or block > blocks_count:
                    print('INVALID', indirection, block, 'IN INODE', str(inode), 'AT OFFSET', offset)
                    inconsistencies = 1
                else:
                    if free_blocks[block] == 1:
                        if referenced_blocks[block] == 0: #avoiding duplicate output
                            print('ALLOCATED BLOCK', block, 'ON FREELIST')
                            inconsistencies = 1
                            referenced_blocks[block] += 1
                    else:
                        referenced_blocks[block] += 1

                if block in reserved_blocks:
                    print('RESERVED', indirection, block, 'IN INODE', str(inode), 'AT OFFSET', offset)
                    inconsistencies = 1

                if block not in duplicate_block_dict:
                    duplicate_block_dict[block] = [[indirection, inode, offset]]
                else: 
                    duplicate_block_dict[block].append([indirection, inode, offset])          
        elif output_type == 'INDIRECT':
            inode = int(fields[1])
            block = int(fields[5])
            indirection_no = int(fields[2])
            offset = int(fields[3])
            if indirection_no == 1:
                indirection = "INDIRECT BLOCK"
            elif indirection_no == 2:
                indirection = "DOUBLE INDIRECT BLOCK"
            elif indirection_no == 3:
                indirection = "TRIPLE INDIRECT BLOCK"

            if block < 0 or block > blocks_count:
                print('INVALID', indirection, block, 'IN INODE', str(inode), 'AT OFFSET', offset)
                inconsistencies = 1
            else:
                if free_blocks[block] == 1:
                    if referenced_blocks[block] == 0: #avoiding duplicate output
                        print('ALLOCATED BLOCK', block, 'ON FREELIST')
                        inconsistencies = 1
                        referenced_blocks[block] += 1
                else:
                    referenced_blocks[block] += 1

            if block in reserved_blocks:
                print('RESERVED', indirection, block, 'IN INODE', str(inode), 'AT OFFSET', offset)
                inconsistencies = 1

            if block not in duplicate_block_dict:
                duplicate_block_dict[block] = [ [indirection, inode, offset] ]
            else: 
                duplicate_block_dict[block].append([indirection, inode, offset])
        elif output_type == 'DIRENT':
            parent_inode = int(fields[1])
            linked_inode = int(fields[3])
            dirent_name = fields[6]
            if debug:
                print(dirent_name)
            if dirent_name == "'.'\n":
                if linked_inode != parent_inode:
                    print('DIRECTORY INODE', parent_inode, "NAME '.' LINK TO INODE", linked_inode, 'SHOULD BE', parent_inode)
                    inconsistencies = 1
            if linked_inode < 0 or linked_inode > inode_count:
                print('DIRECTORY INODE', parent_inode, 'NAME', dirent_name[:-1], 'INVALID INODE', linked_inode)
                inconsistencies = 1
                continue
            if dirent_name != "'.'\n" and dirent_name != "'..'\n":
                inode_par_array[linked_inode] = parent_inode
            if dirent_name == "'..'\n":
                dotdot_entry_dict[parent_inode] = linked_inode

            inode_linkcount_counter[linked_inode] += 1
            inode_to_dirname_linkedInode_dict[inode] = [dirent_name, linked_inode]


    added_free_referenced_block_array = list(map(add,free_blocks,referenced_blocks))
    added_free_allocated_inode_array = list(map(add,free_inodes,allocated_inodes))
    if debug:
        print("BLOCKS:")
        print("Free  : {}".format(free_blocks))
        print("Ref.  : {}".format(referenced_blocks))
        print("Added : {}".format(added_free_referenced_block_array))
        print("INODES:")
        print("Free  : {}".format(free_inodes))
        print("Alloc : {}".format(allocated_inodes))
        print("Added : {}".format(added_free_allocated_inode_array))
        print("Dirname_Link_Dict:", inode_to_dirname_linkedInode_dict)
        print("Inode_par_array:", inode_par_array)
        print("Dotdot_entry_dict:", dotdot_entry_dict)
    
    for i in range(0,blocks_count):
        if added_free_referenced_block_array[i] == 0 and i not in reserved_blocks:
            print('UNREFERENCED BLOCK', i)
            inconsistencies = 1

    for i in range(0,inode_count):
        if free_inodes[i] != 0 and allocated_inodes[i] != 0:
            print('ALLOCATED INODE', i, 'ON FREELIST')
            inconsistencies = 1
        if free_inodes[i] == 0 and i not in reserved_inodes and allocated_inodes[i] == 0:
            print('UNALLOCATED INODE', i, 'NOT ON FREELIST')
            inconsistencies = 1

    for block in duplicate_block_dict:
        if len(duplicate_block_dict[block]) > 1:
            if debug:
                print("Duplicate for block:", block)
            for block_reference in duplicate_block_dict[block]:
                print('DUPLICATE', block_reference[0], block, 'IN INODE', block_reference[1], 'AT OFFSET', block_reference[2])
                inconsistencies = 1

    for i in range(0,inode_count):
        if i in inode_linkcount_dict.keys():
            linkcount = inode_linkcount_dict[i]
        else:
            linkcount = 0
        links = inode_linkcount_counter[i]

        if links != linkcount:
            print('INODE', i, 'HAS', links, 'LINKS BUT LINKCOUNT IS' , linkcount)
            inconsistencies = 1

    for i in range(0,inode_count):
        if i in inode_to_dirname_linkedInode_dict.keys():
            dirent_name = inode_to_dirname_linkedInode_dict[i][0]
            linked_inode = inode_to_dirname_linkedInode_dict[i][1]
            if debug:
                print("Name: {}, Inode: {}".format(dirent_name, linked_inode))
            if allocated_inodes[linked_inode] == 0:
                print('DIRECTORY INODE', i, 'NAME' ,dirent_name[:-1], 'UNALLOCATED INODE', linked_inode)
                inconsistencies = 1

    #dotdot_entry_inode is the inode to look at
    #dotdot_entry_dict gives its parent that it points at in ..-entry
    for dotdot_entry_inode in dotdot_entry_dict:
        if dotdot_entry_inode == 2:
            if dotdot_entry_dict[2] != 2:
                print('DIRECTORY INODE', 2, "NAME '..' LINK TO INODE", dotdot_entry_dict[2], 'SHOULD BE', 2)
                inconsistencies = 1
        #Now checking whether the parent actually points to the child
        #inode_par_array saves all non-..-parent-to-child-relationships: Given child it outputs parent
        elif dotdot_entry_dict[dotdot_entry_inode] != inode_par_array[dotdot_entry_inode]:
            child = dotdot_entry_inode
            parent = inode_par_array[dotdot_entry_inode]
            link = dotdot_entry_dict[dotdot_entry_inode]
            print('DIRECTORY INODE', child, "NAME '..' LINK TO INODE", link, 'SHOULD BE', parent)
            inconsistencies = 1



    if inconsistencies == 0:
        exit(0)
    else:
        exit(2)

if __name__ == "__main__":
    main()
