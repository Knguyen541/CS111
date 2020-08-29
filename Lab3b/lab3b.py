#!/usr/bin/python

import sys

def main():
    debug = 0
    reserved_blocks = set([0,1])
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
            free_blocks = [1]*(blocks_count+1)
            free_inodes = [1]*(inode_count+1)
            if fields[3] == '1024':
                reserved_blocks.add(2)
            if debug:
                print("Blocks_count: {}, Inode_count: {}".format(blocks_count, inode_count))
        elif output_type == 'GROUP':
            reserved_blocks.add(int(fields[6]))
            reserved_blocks.add(int(fields[7]))
            reserved_blocks.add(int(fields[8]))
            if debug:
                print("Reserved blocks: {}".format(reserved_blocks))
        elif output_type == 'BFREE':
            free_blocks[int(fields[1])] = 0
        elif output_type == 'IFREE':
            free_inodes[int(fields[1])] = 0

    if debug:
        print("Free block array: {}".format(free_blocks))
        print("Free inode array: {}".format(free_inodes))
            

if __name__ == "__main__":
    main()
