#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

#define TABLE_ROW_COUNT 512
#define FILE_SIZE 512
#define BLOCK_SIZE 1024
#define ROW_SIZE 40
#define CONTENT_BUFFER_SIZE 515
#define NAME_LEN 10
const char *MyFs::MYFS_MAGIC = "MYFS";
int memory_location = 0;

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);
	inode_table.reserve(TABLE_ROW_COUNT);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::format() 
{
	int i = 0, address_offset = sizeof(myfs_header);
	dir_list root_directory;

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);
	size_t j = 0;

	// initialize all inodes in table
	for(i = 0; i < TABLE_SIZE; i++)
	{
		inode_table[i].inode_num = i;
		inode_table[i].address= 0;
		inode_table[i].size = 0;
	}

	// write all inodes to block device
	for(i = 0 ; i < TABLE_SIZE; i++)
	{
		address_offset = sizeof(myfs_header);
		// write inode num (int)
		blkdevsim->write( i * ROW_SIZE + address_offset, sizeof(inode_table[i].inode_num),  (const char*)&inode_table[i].inode_num);
		address_offset += sizeof(inode_table[i].inode_num);
		// write address (chars)
		blkdevsim->write( i * ROW_SIZE + address_offset, sizeof(inode_table[i].address),  (const char*)&inode_table[i].address);
		address_offset += sizeof(inode_table[i].address);
		// write size(int)
		blkdevsim->write( i * ROW_SIZE + address_offset, sizeof(inode_table[i].size),  (const char*)&inode_table[i].size);
	}

	// write root directory to block
	for (j = 0; j < root_directory.size() ; j++)
	{
		address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);
		// write if is directory
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].is_dir);
		address_offset += sizeof(root_directory[j].is_dir);
		// write name
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].name),  (const char*)&root_directory[j].name);
		address_offset += sizeof(root_directory[j].name);
		// write file size
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].file_size);
	}

}

void MyFs::create_file(std::string path_str, bool directory) 
{
	std::cout << "Creating File..." << std::endl;
	dir_list root_directory;
	char path[NAME_LEN];
	bool is_dir = false;
	int i = 0, file_size = 0, address_offset = sizeof(myfs_header);
	size_t j = 0;

	// read root directory from block device
	for(i = 0; path[i] !=  0; i++)
	{
		address_offset = sizeof(myfs_header);
		// read name
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(path), path);
		address_offset += sizeof(path);
		// read isDir
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(is_dir), (char*)&is_dir);
		address_offset += sizeof(is_dir);
		// read file size
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_size), (char*)&file_size);
		root_directory.push_back({path, is_dir, file_size});

	}
	// add new file to root directory 
	root_directory.push_back({path_str, directory, 0});

	// write root directory to block
	for (j = 0; j < root_directory.size() ; j++)
	{
		address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);
		// write if is directory
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].is_dir);
		address_offset += sizeof(root_directory[j].is_dir);
		// write name
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].name),  (const char*)&root_directory[j].name);
		address_offset += sizeof(root_directory[j].name);
		// write file size
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].file_size);
	}

	// read all inodes from block device
	for(i = 0 ; i < TABLE_SIZE; i++)
	{
		address_offset = sizeof(myfs_header);
		// read inode num (int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].inode_num),(char*)&inode_table[i].inode_num);
		address_offset += sizeof(inode_table[i].inode_num);
		// read address (chars)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].address),(char*)&inode_table[i].address);
		address_offset += sizeof(inode_table[i].address);
		// read size(int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].size),(char*)&inode_table[i].size);
	}

	// search for and initialize empty rows in inode table
	for( i = 0; i < TABLE_SIZE; i++)
	{
		if (inode_table[i].address == 0)
		{
			inode_table[i].size = 0;
			inode_table[i].inode_num = i;
			inode_table[i].address = 0;
			break;
		}
	}

	// write root directory to block
	for (j = 0; j < root_directory.size() ; j++)
	{
		address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);address_offset = sizeof(myfs_header);
		// write if is directory
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].is_dir);
		address_offset += sizeof(root_directory[j].is_dir);
		// write name
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].name),  (const char*)&root_directory[j].name);
		address_offset += sizeof(root_directory[j].name);
		// write file size
		blkdevsim->write(BLOCK_SIZE + i * ROW_SIZE + address_offset, sizeof(root_directory[j].is_dir),  (const char*)&root_directory[j].file_size);
	}
}


std::string MyFs::get_content(std::string path_str) {
	char file_name[NAME_LEN], buffer[CONTENT_BUFFER_SIZE];
	bool is_dir = false;
	int i = 0, file_size = 0, address_offset = sizeof(myfs_header);
	size_t found_id = 0 ;
	dir_list root_directory;
	std::string content;

	std::cout << "Getting file content..." << std::endl;

	// read root directory from block device
	for(i = 0; file_name[i] !=  0; i++)
	{
		address_offset = sizeof(myfs_header);
		// read name
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_name), file_name);
		address_offset += sizeof(file_name);
		// read isDir
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(is_dir), (char*)&is_dir);
		address_offset += sizeof(is_dir);
		// read file size
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_size), (char*)&file_size);
		root_directory.push_back({file_name, is_dir, file_size});

	}

	// find file in root directory
	for (found_id = 0; found_id < root_directory.size(); found_id++)
	{
		if(root_directory[found_id].name == path_str) { break;}
	}

	// read all inodes from block device
	address_offset = sizeof(myfs_header);
	for(i = 0 ; i < TABLE_SIZE; i++)
	{
		address_offset = sizeof(myfs_header);
		// read inode num (int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].inode_num),(char*)&inode_table[i].inode_num);
		address_offset += sizeof(inode_table[i].inode_num);
		// read address (chars)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].address),(char*)&inode_table[i].address);
		address_offset += sizeof(inode_table[i].address);
		// read size(int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].size),(char*)&inode_table[i].size);
	}
	address_offset = sizeof(myfs_header);
	// read file content
	blkdevsim->read(inode_table[found_id].inode_num * FILE_SIZE + address_offset, FILE_SIZE, buffer);
	content = buffer;
	return content;
}

void MyFs::set_content(std::string path_str, std::string content) {
	char file_name[NAME_LEN];
	bool is_dir = false;
	int i = 0, file_size = 0, address_offset = sizeof(myfs_header);
	size_t found_id = 0;
	dir_list root_directory;
	
	std::cout << "Setting file content..." << std::endl;

	// read root directory from block device
	for(i = 0; file_name[i] !=  0; i++)
	{
		address_offset = sizeof(myfs_header);
		// read name
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_name), file_name);
		address_offset += sizeof(file_name);
		// read isDir
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(is_dir), (char*)&is_dir);
		address_offset += sizeof(is_dir);
		// read file size
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_size), (char*)&file_size);
		root_directory.push_back({file_name, is_dir, file_size});

	}
	// find file in root directory
	for (found_id = 0; found_id < root_directory.size(); found_id++)
	{
		if(root_directory[found_id].name == path_str) { break;}
	}

	// read all inodes from block device
	for(i = 0 ; i < TABLE_SIZE; i++)
	{
		address_offset = sizeof(myfs_header);
		// read inode num (int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].inode_num),(char*)&inode_table[i].inode_num);
		address_offset += sizeof(inode_table[i].inode_num);
		// read address (chars)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].address),(char*)&inode_table[i].address);
		address_offset += sizeof(inode_table[i].address);
		// read size(int)
		blkdevsim->read(i*ROW_SIZE + address_offset, sizeof(inode_table[i].size),(char*)&inode_table[i].size);
	}
	address_offset = sizeof(myfs_header);
	// write new file size to inode table
	inode_table[found_id].size = content.size();
	blkdevsim->write(i*ROW_SIZE + address_offset, sizeof(inode_table[i].size), (char*)&inode_table[i].size);
	// write the content
	blkdevsim->write(inode_table[i].inode_num + address_offset, FILE_SIZE, content.c_str());
	
}

MyFs::dir_list MyFs::list_dir(std::string path_str) {
	dir_list root_directory;
	std::cout << "Listing directory:" << std::endl;
	char file_name[NAME_LEN];
	bool is_dir = false;
	int i = 0, file_size = 0, address_offset = sizeof(myfs_header);
	// read root directory from block device
	for(i = 0; file_name[i] !=  0; i++)
	{
		address_offset = sizeof(myfs_header);
		// read name
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_name), file_name);
		address_offset += sizeof(file_name);
		// read isDir
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(is_dir), (char*)&is_dir);
		address_offset += sizeof(is_dir);
		// read file size
		blkdevsim->read(BLOCK_SIZE + i*ROW_SIZE + address_offset, sizeof(file_size), (char*)&file_size);
		root_directory.push_back({file_name, is_dir, file_size});

	}
	return root_directory;
}

