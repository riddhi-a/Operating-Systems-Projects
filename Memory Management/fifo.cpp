#include <bits/stdc++.h>
#include <cstring>
#include <fstream>
#include <getopt.h>
using namespace std;
string infile1;
string outfile1;
int PAGE_SIZE;
int MAIN_MEMORY_SIZE;
int SWAP_MEMORY_SIZE;
int MAIN_MEMORY_PAGES;
int SWAP_MEMORY_PAGES;
struct Page {
    vector<int> content;
    int pid;
    int logical_page_number;
    int tou;

    Page(int pid, int vpn, int tou)
        : content(PAGE_SIZE, 0), pid(pid), logical_page_number(vpn), tou(tou) {}
};

int global_time_counter = 0;
vector<Page*> main_memory;
vector<Page*> swap_memory;
set<int> free_main_pages;
set<int> free_swap_pages;
set<pair<int, int>> fifo_set;

unordered_map<int, string> spid_to_filename;
map<int, vector<pair<int, int>>> pid_to_page_map;
void replace_page_fifo(int pid, int swap_page_number, int logical_page_number) {
    
    if (fifo_set.empty()) return;
    
    auto fifo = *fifo_set.begin();
    int fifo_page_number = fifo.second;
    fifo_set.erase(fifo_set.begin());
    
    Page* fifo_page = main_memory[fifo_page_number];
    int remove_pid = fifo_page->pid;
    int remove_logical_page = fifo_page->logical_page_number;
    Page* desired_page = swap_memory[swap_page_number];

     free_swap_pages.erase(swap_page_number);
    swap_memory[swap_page_number] = fifo_page;
    pid_to_page_map[remove_pid][remove_logical_page] = make_pair(swap_page_number, 0);
    
    main_memory[fifo_page_number] = desired_page;
    main_memory[fifo_page_number]->tou = global_time_counter++;
    pid_to_page_map[pid][logical_page_number] = make_pair(fifo_page_number, 1);
    
    fifo_set.insert({main_memory[fifo_page_number]->tou, fifo_page_number});
     
}

int* resolve_address(int pid, int logical_address) {
    if (pid_to_page_map.find(pid) == pid_to_page_map.end()) return nullptr;
    auto& page_table = pid_to_page_map[pid];
    int page_number = logical_address / PAGE_SIZE;
    int offset = logical_address % PAGE_SIZE;

    if (page_number >= page_table.size()) 
    {
        ofstream outf(outfile1, ios::app);
        outf<<"Invalid Memory Address "<<logical_address<<" specified for process id "<<pid<<endl;
        outf.close();
        return nullptr;
    }
    int physical_page = page_table[page_number].first;
    int  flag = page_table[page_number].second;

    if (physical_page == -1) return nullptr;

    if ( flag == 1) {
        return &main_memory[physical_page]->content[offset];
    } else {
        if (!free_main_pages.empty()) {
           
            int physical_page_number = *free_main_pages.begin();
            free_main_pages.erase(free_main_pages.begin());
            free_swap_pages.insert(physical_page);
            Page* t =  main_memory[physical_page_number];
            fifo_set.erase({main_memory[physical_page_number]->tou, physical_page_number});
            main_memory[physical_page_number] = swap_memory[physical_page];
            swap_memory[physical_page]=t;
            pid_to_page_map[pid][page_number] = make_pair(physical_page_number, 1);
            main_memory[physical_page_number]->tou=global_time_counter++;
            fifo_set.insert({main_memory[physical_page_number]->tou, physical_page_number});
           
            return &main_memory[pid_to_page_map[pid][page_number].first]->content[offset];
             
        }
        else{
        replace_page_fifo(pid, physical_page, page_number);
        return &main_memory[pid_to_page_map[pid][page_number].first]->content[offset];
        }
        
    }
    return nullptr;
}

void load_executable(const string& filename, int& pid_counter) {
    ifstream infile(filename);
    ofstream outf(outfile1, ios::app);
    if (!infile.is_open()) 
    {
        outf<<filename<<" could not be loaded - file does not exist\n";
        outf.close();
        return;
    }
    int executable_size;
    infile >> executable_size;
    int required_pages = (executable_size * 1024 + PAGE_SIZE - 1) / PAGE_SIZE;

    if ((free_main_pages.size() + free_swap_pages.size()) < required_pages)
    {
        outf<<filename<<" could not be loaded - memory is full\n";
        outf.close();
        return;
    }

    int pid = pid_counter++;
    spid_to_filename[pid] = filename;
    pid_to_page_map[pid] = vector<pair<int, int>>(required_pages, make_pair(-1, 0));

    for (int i = 0; i < required_pages; i++) {
        int physical_page_number = -1;
        int flag = 0;

        if (!free_main_pages.empty()) {
            physical_page_number = *free_main_pages.begin();
            free_main_pages.erase(free_main_pages.begin());
            flag = 1;
        } else if (!free_swap_pages.empty()) {
            physical_page_number = *free_swap_pages.begin();
            free_swap_pages.erase(free_swap_pages.begin());
        }

        if (physical_page_number != -1 && flag == 1) {
           
            fifo_set.erase({main_memory[physical_page_number]->tou, physical_page_number});
            main_memory[physical_page_number]->pid = pid;
            main_memory[physical_page_number]->tou = global_time_counter++;
            main_memory[physical_page_number]->logical_page_number = i;
            pid_to_page_map[pid][i] = make_pair(physical_page_number, flag);
            fifo_set.insert({main_memory[physical_page_number]->tou, physical_page_number});
            
        } else if (physical_page_number != -1) {
            swap_memory[physical_page_number]->pid = pid;
            swap_memory[physical_page_number]->logical_page_number = i;
            pid_to_page_map[pid][i] = make_pair(physical_page_number, flag);
        }
    }
    outf<<filename<<" is loaded and is assigned process id "<<pid<<"\n";
    outf.close();
}

void kill_process(int pid) {
    ofstream outf(outfile1, ios::app);
    if (pid_to_page_map.find(pid) == pid_to_page_map.end()) 
    {
         outf<< "Invalid PID "<<pid<<endl;
        outf.close();
        return;
    }
    auto& page_table = pid_to_page_map[pid];
    for (int i = 0; i < page_table.size(); i++) {
        int physical_page = page_table[i].first;
        int flag = page_table[i].second;
        if (physical_page == -1) continue;
        if (flag == 1) 
        free_main_pages.insert(physical_page);
        else free_swap_pages.insert(physical_page);
    }
    pid_to_page_map.erase(pid);
    outf<<"killed "<<pid<<endl;
    outf.close();
}

void run_process(int pid) {
    ofstream outf(outfile1, ios::app);
    if (pid_to_page_map.find(pid) == pid_to_page_map.end())
    {
         outf<< "Invalid PID "<<pid<<endl;
        outf.close();
        return;
    }
    string filename = spid_to_filename[pid];
    ifstream infile(filename);
    if (!infile.is_open()) return;
    int executable_size;
    infile >> executable_size;

    string command;
    while (getline(infile, command)) {
        stringstream ss(command);
        string operation;
        ss >> operation;

        if (operation == "load") {
            int value, logical_address;
            char sep;
            ss >> value>>sep >> logical_address;
            int* addr = resolve_address(pid, logical_address);
            if(addr==nullptr) return;
            
                *addr =  (value);
                outf << "Command: load " << value << ", " << logical_address << "; "
                     << "Result: Value of " << value << " is now stored in addr " << logical_address  << endl;
            
        } else if (operation == "add") {
            int x, y, z;char sep;
            ss >> x>>sep >> y>>sep>>z;
            int* addr_x = resolve_address(pid, x);
            if(addr_x==nullptr) return;
            int val_x= (*addr_x);
            int* addr_y = resolve_address(pid, y);
            if(addr_y==nullptr) return;
             int val_y= (*addr_y);
            int* addr_z = resolve_address(pid, z);
            if(addr_z==nullptr) return;
            
                *addr_z = val_x + val_y;
                int val_z = *addr_z;
                outf << "Command: add " << x << ", " << y << ", " << z << "; "
                     << "Result: Value in addr " << x << " = " <<  (val_x)
                     << ", addr " << y << " = " <<  (val_y)
                     << ", addr " << z << " = " <<  (val_z) << endl;
            
        } else if (operation == "sub") {
            int x, y, z;char sep;
            ss >> x>>sep >> y>>sep>>z;
            int* addr_x = resolve_address(pid, x);
            if(addr_x==nullptr) return;
            int val_x= (*addr_x);
            int* addr_y = resolve_address(pid, y);
            if(addr_y==nullptr) return;
             int val_y= (*addr_y);
            int* addr_z = resolve_address(pid, z);
            if(addr_z==nullptr) return;
            
                *addr_z = val_x - val_y;
                int val_z = *addr_z;
                outf << "Command: sub " << x << ", " << y << ", " << z << "; "
                     << "Result: Value in addr " << x << " = " <<  (val_x)
                     << ", addr " << y << " = " <<  (val_y)
                     << ", addr " << z << " = " <<  (val_z) << endl;
            
        } else if (operation == "print") {
            int logical_address;
            ss >> logical_address;

            int* addr = resolve_address(pid, logical_address);
            if(addr==nullptr) return;
            
                outf << "Command: print " << logical_address << "; "
                     << "Result: Value in addr " << logical_address << " = " <<  (*addr) << endl;
            
        }
    }
    outf.close();
}

void pte_pid(int pid, const string& filename) {
    ofstream outf(outfile1, ios::app);
    ofstream outfile(filename, ios::app);
    if (pid_to_page_map.find(pid) == pid_to_page_map.end()) {
         outf<< "Invalid PID "<<pid<<endl;
        outf.close();
        return;
    }
    outf.close();
    system(("date >> " + filename).c_str());
    int c = 0;
    for (auto& [physical_page, entry] : pid_to_page_map[pid]) {
        outfile << c << " " << physical_page << " " << entry << endl;
        c++;
    }
    outfile.close();
}

void pte_all(const std::string& filename) {
    system(("date >> " + filename).c_str());
    ofstream outfile(filename, ios::app);
    int c = 0;
    for (auto& [pid, table] : pid_to_page_map) {
        c = 0;
        for (auto& [physical_page, entry] : pid_to_page_map[pid]) {
            outfile <<pid<<" "<< c << " " << physical_page << " " << entry << endl;
            c++;
        }
    }
    outfile.close();
}

void print_memory(int memloc, int length) {
    ofstream outf(outfile1, ios::app);
    for (int i = memloc; i < memloc + length; i++) {
        if (i >= 0 && i < MAIN_MEMORY_PAGES * PAGE_SIZE) {
            int page_number = i / PAGE_SIZE;
            int offset = i % PAGE_SIZE;
            if (main_memory[page_number]) outf <<"Value of "<<i<<": "<< (int)main_memory[page_number]->content[offset] << endl;
        }
        else
        {
          outf <<"Invalid Memory Access\n";
          outf.close();
          return;
        }
    }
    outf.close();
}

void listpr() {
    ofstream outf(outfile1, ios::app);
    for (auto pid : pid_to_page_map) outf << pid.first << " ";
    outf << endl;
    outf.close();
}

void exit() {
    for (auto page : main_memory) delete page;
    for (auto page : swap_memory) delete page;
    free_main_pages.clear();
    free_swap_pages.clear();
    spid_to_filename.clear();
    pid_to_page_map.clear();
    fifo_set.clear();
}
int main(int argc, char* argv[]) {
	
	struct option longopts[] = {
	  {"i",     required_argument,       0, 'a'},
      {"o",  required_argument,       0, 'b'},
      {"M",  required_argument, 0, 'c'},
      {"V",  required_argument, 0, 'd'},
      {"P",  required_argument, 0, 'e'},{0,0,0,0}
    };

    while(1){
    	const int opt = getopt_long(argc, argv, "", longopts, 0);

    	if (opt == -1){
    		break;
    	}

    	switch (opt){
    	case 'a':
          infile1=optarg;
          break;

        case 'b':
          outfile1=optarg;
          break;

        case 'c':
            MAIN_MEMORY_SIZE = stoi(optarg)*1024;
          break;
        
        case 'd':
            SWAP_MEMORY_SIZE = stoi(optarg)*1024;
          break;

        case 'e':
            PAGE_SIZE = stoi(optarg);
          break;
    	case '?':
    		abort ();
		default:
			abort ();
    	}
    }
    MAIN_MEMORY_PAGES = (MAIN_MEMORY_SIZE / PAGE_SIZE);
    SWAP_MEMORY_PAGES = (SWAP_MEMORY_SIZE / PAGE_SIZE);
    main_memory.resize(MAIN_MEMORY_PAGES, nullptr);
    swap_memory.resize(SWAP_MEMORY_PAGES, nullptr);
    for (int i = 0; i < MAIN_MEMORY_PAGES; ++i) 
    {
      Page* new_page = new Page(-1, -1, global_time_counter++);
      main_memory[i]=new_page;
      fifo_set.insert({main_memory[i]->tou, i});
      free_main_pages.insert(i);
    }
    for (int i = 0; i < SWAP_MEMORY_PAGES; ++i)
    {
     Page* new_page = new Page(-1, -1, 0);
     swap_memory[i]=new_page;
     free_swap_pages.insert(i);
    }

    int pid_counter = 1;
    ifstream infile(infile1);
    ofstream outfile(outfile1);
    string command;

    if (!infile.is_open() || !outfile.is_open()) {
        cout << "Failed to open infile or outfile." << endl;
        return 1;
    }

    while (getline(infile, command)) {
        stringstream ss(command);
        string operation;
        ss >> operation;

        if (operation == "load") {
            string filename;
            //cout<<"loadstart";
            while (ss >> filename) {
                load_executable(filename, pid_counter);
            }
             //cout<<"loadend";
        } else if (operation == "run") {
            int pid;
            ss >> pid;
            //cout<<"runstart";
            run_process(pid);
            //cout<<"runend";
        } else if (operation == "kill") {
            int pid;
            ss >> pid;
            //cout<<"killstart";
            kill_process(pid);
            //cout<<"killend";
        } else if (operation == "listpr") {
            //cout<<"prstart";
            listpr();
            //cout<<"prend";
        } else if (operation == "pte") {
            int pid;
            string outfilename;
            ss >> pid >> outfilename;
            //cout<<"ptestart";
            pte_pid(pid, outfilename);
            //cout<<"pteend";
        } else if (operation == "pteall") {
            string outfilename;
            ss >> outfilename;
            pte_all(outfilename);
        } else if (operation == "print") {
            int memloc, length;
            ss >> memloc >> length;
             //cout<<"start";
           print_memory(memloc,length);
            //cout<<"end";
        } else if (operation == "exit") {
            exit();
            break;
        }
    }
    infile.close();
    outfile.close();
    return 0;
}
