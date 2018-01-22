#include "filter.hh"

#include "mpi_wrappers.hh"
#include "utils.hh"
#include "pgm_image.hh"

#include <fstream>
#include <algorithm>
#include <map>

#define TOPOLOGY_TAG 1
#define COMMAND_TAG 2

std::map<std::string, std::vector<int> > filter_mapping = 
{
    {"mean_removal", {-1, -1, -1, -1, 9, -1, -1, -1, -1, 1, 0}},
    {"sobel", {1, 0, -1, 2, 0, -2, 1, 0, -1, 1, 127}},
    {"sharpen", {0, -1, 0, -1, 5, -1, 0, -1, 0, 1, 0}}
};

void MergeProbes(int *probe, int *response, int n_procs)
{
    for (int j = 0; j < n_procs; j++)
    {
        if (response[j] != INVALID_ID)
        {
            probe[j] = response[j];
        }
    }
}

void ReadTopology(const char *topology_filename)
{
    std::string line;
    int node_id = INVALID_ID;
    std::vector<std::string> adj_list;

    /* Open & parse topology file */
    std::ifstream topology_file(topology_filename);
    if (!topology_file.is_open())
    {
        ThrowError("Could not open topology file");
    }

    while (std::getline(topology_file, line))
    {
        /* Ignore lines not corresponding to my rank (aka I only read my adjlist) */
        node_id = std::stoi(line.substr(0, line.find(":")));
        if (node_id == rank)
        {
            /* split line by space and convert tokens to integers */
            adj_list = SplitString(line.substr(line.find(":") + 2), ' ');
            std::transform(adj_list.begin(), adj_list.end(), std::back_inserter(neighbours), [](std::string s) { return std::stoi(s); });
        }
    }

    topology_file.close();
}

void ComputeTree()
{
    int *probe = new int[n_procs + 1];
    int *response = new int[n_procs + 1];

    const int sender = n_procs;

    if (ROOT_ID == rank)
    {
        /* Prepare probe */
        for (int i = 0; i < n_procs; i++)
        {
            probe[i] = INVALID_ID;
        }
        probe[sender] = rank;

        /* Send probe to all neighbours */
        for(unsigned int i = 0; i < neighbours.size(); i ++) {
            MPI_Isend(probe, n_procs + 1, MPI_INT, neighbours[i], TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
        // SendAll(probe, n_procs + 1, MPI_INT, neighbours);
        for (unsigned int i = 0; i < neighbours.size(); i++)
        {
            /* Receive response from all neighbours */
            MPI_Irecv(response, n_procs + 1, MPI_INT, MPI_ANY_SOURCE, TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            // Recv(response, n_procs + 1, MPI_INT, MPI_ANY_SOURCE);

            /* Update probe */
            MergeProbes(probe, response, n_procs);
        }
    }
    else
    {
        /* Receive probe from someone */
        MPI_Irecv(probe, n_procs + 1, MPI_INT, MPI_ANY_SOURCE, TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        // Recv(probe, n_procs + 1, MPI_INT, MPI_ANY_SOURCE);

        /* Mark sender as parent */
        parent = probe[sender];

        /* Update probe */
        probe[rank] = parent;

        /* Mark me as sender */
        probe[sender] = rank;

        for (unsigned int i = 0; i < neighbours.size(); i++)
        {
            /* Forward probe to all neighbours except parent */
            if (neighbours[i] == parent)
            {
                continue;
            }

            // Send(probe, n_procs + 1, MPI_INT, neighbours[i]);
            MPI_Isend(probe, n_procs + 1, MPI_INT, neighbours[i], TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        }

        for (unsigned int i = 0; i < neighbours.size() - 1; i++)
        {
            /* Receive response from all neighbours(except one, my parent) */
            MPI_Irecv(response, n_procs + 1, MPI_INT, MPI_ANY_SOURCE, TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            // Recv(response, n_procs + 1, MPI_INT, MPI_ANY_SOURCE);

            /* Update probe */
            MergeProbes(probe, response, n_procs);
        }

        /* Send probe back to parent */
        MPI_Isend(probe, n_procs + 1, MPI_INT, parent, TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        // Send(probe, n_procs + 1, MPI_INT, parent);
    }


    /* The correct probe is only in root now, so we need to
    propagate it to the rest of the tree.*/

    if (ROOT_ID == rank)
    {
        /* Send probe to all children */
        // SendAll(probe, n_procs + 1, MPI_INT, neighbours);
        for(unsigned int i = 0; i < neighbours.size(); i++) {
            MPI_Isend(probe, n_procs + 1, MPI_INT, neighbours[i], TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
    }
    else
    {
        /* Receive probe from parent */
        MPI_Irecv(probe, n_procs + 1, MPI_INT, parent, TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        for (unsigned int i = 0; i < neighbours.size(); i++)
        {
            /* Send probe to all neighbours except parent */
            if (neighbours[i] == parent)
            {
                continue;
            }
            // Send(probe, n_procs + 1, MPI_INT, neighbours[i]);
            MPI_Isend(probe, n_procs + 1, MPI_INT, neighbours[i], TOPOLOGY_TAG, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
    }

    /* Each node extracts a list with its children */
    for (int i = 0; i < n_procs; i++)
    {
        if (probe[i] == rank)
        {
            children.push_back(i);
        }
    }

    delete[] response;
    response = NULL;
    
    delete[] probe;
    probe = NULL;
}

void Leader(const char *image_specification_filename, const char *statistics_filename)
{
    std::ifstream image_specification_file;
    std::vector<std::string> filter_names;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::string> tokens;
    std::vector<std::string> comments;
    std::string format;
    int grayscale;

    int image_count;

    std::string line;

    /* Read image specification file */
    image_specification_file.open(image_specification_filename);
    if (!image_specification_file.is_open()) {
        ThrowError("Could not open image specification file.");
    }

    /* Read image count */
    std::getline(image_specification_file, line);
    image_count = std::stoi(line);

    /* Read input, filter and output names */
    for(int i = 0; i < image_count; i++)
    {
        std::getline(image_specification_file, line);
        tokens = SplitString(line, ' ');

        filter_names.push_back(tokens[0]);
        input_names.push_back(tokens[1]);
        output_names.push_back(tokens[2]);
    }
    image_specification_file.close();

    for(int i = 0; i < image_count; i++)
    {

        /* Send filter name */
        strcpy(message, filter_names[i].c_str());
        SendAll(message, COMMAND_BUFFER_SIZE, MPI_CHAR, children, COMMAND_TAG);

        /* Read each image */
        PGMImage img = ReadImage(input_names[i]);
        image_width = img.n_col;
        buffer_size = img.n_row;
        comments = img.comments;
        format = img.format;
        grayscale = img.grayscale;

        /* Compute each child's strip size */
        int *child_buffer_sizes = new int[children.size()]();
        std::vector<std::pair<int, int>> limits = GetChildBorders(buffer_size);

        /* Allocate buffer to hold image */
        buffer = new int[image_width * buffer_size];
        for(int j = 0; j < buffer_size; j++)
        {
            memcpy(&buffer[j*image_width], img.image[j], sizeof(*buffer)*image_width);
        }
        img.FreeMemory();

        /* Send to children */
        int child_strip_size = 0;
        int strip_start = 0;
        int active_children = 0;
        for(unsigned int j = 0; j < children.size(); j++)
        {
            /* first send image width */
            Send(&image_width, 1, MPI_INT, children[j]);

            /* then send strip size (in lines) */
            if (j < limits.size()) {
                child_strip_size = limits[j].second - limits[j].first + 1;
            }
            child_buffer_sizes[j] = child_strip_size;

            Send(&child_strip_size, 1, MPI_INT, children[j]);

            /* If buffer size > 0, send buffer as well */
            if (child_strip_size > 0 ) {
                strip_start = limits[j].first * image_width;
                Send(&buffer[strip_start], child_strip_size * image_width, MPI_INT, children[j]);
            }
        }

        /* Receive results */
        active_children = 0;
        for (int k = 0; k < (int) children.size(); k++)
        {
            if (child_buffer_sizes[k] > 0)
            {
                active_children++;
            }
        }
        for(int j = 0; j < (int) children.size(); j++) {
            if (child_buffer_sizes[j] > 0)
            {
                processed_buffer = new int[child_buffer_sizes[j] * image_width]();
                Recv(processed_buffer, child_buffer_sizes[j] * image_width, MPI_INT, children[j]);

                if (active_children == 1 && j == 0)
                {
                    memcpy(buffer, processed_buffer, (child_buffer_sizes[j] * image_width) * sizeof(*buffer));
                }
                else if (j == 0)
                {
                    memcpy(buffer, processed_buffer, sizeof(*buffer) * ((child_buffer_sizes[j] - 1) * image_width));
                }
                else if (j == (int) active_children - 1)
                {
                    int buf_start_idx = (limits[j].first + 1) * image_width;
                    int buf_stop_idx = buf_start_idx + (child_buffer_sizes[j] - 1) * image_width;
                    for (int k = buf_start_idx; k < buf_stop_idx; k++)
                    {
                        buffer[k] = processed_buffer[k - buf_start_idx + image_width];
                    }
                }
                else
                {
                    int buf_start_idx = (limits[j].first + 1) * image_width;
                    int buf_stop_idx = buf_start_idx + (child_buffer_sizes[j] - 2) * image_width;

                    for (int k = buf_start_idx; k < buf_stop_idx; k++)
                    {
                        buffer[k] = processed_buffer[k - buf_start_idx + image_width];
                    }
                }
                delete[] processed_buffer;
            }
        }

        PGMImage output_image(buffer_size - 2, image_width - 2);
        output_image.comments = comments;
        output_image.format = format;
        output_image.grayscale = grayscale;
        for(int j = 1; j < buffer_size - 1; j++) {
                memcpy(output_image.image[j-1], &buffer[j*image_width + 1], sizeof(*buffer)*(image_width - 1));
        }
        delete[] buffer;
        buffer = NULL;
        WriteImageToFile(output_names[i], output_image);

        output_image.FreeMemory();
    }

    /* Send communication end & recv statistics */
    strcpy(message, "end");
    SendAll(message, COMMAND_BUFFER_SIZE, MPI_CHAR, children, COMMAND_TAG);

    statistic = new int[n_procs]();
    int *response = new int[n_procs]();

    for(unsigned int i = 0; i < children.size(); i++)
    {
        Recv(response, n_procs, MPI_INT, children[i]);

        for(int j = 0; j < n_procs; j++)
        {
            if (response[j] != 0)
            {
                statistic[j] = response[j];
            }
        }
    }
    delete[] response;
    response = NULL;
    
    /* write statistics to output file */
    std::ofstream statistics_file;
    statistics_file.open(statistics_filename);
    if(!statistics_file.is_open()) {
        ThrowError("Could not open statistics file.");
    }
    for(int i = 0; i < n_procs; i++) {
        statistics_file << i << ": " << statistic[i] << "\n";
    }
    statistics_file.close();
    delete[] statistic;
    statistic = NULL;

}

void Intermediary() 
{
    int *response = new int[n_procs]();
    while(1) {

        Recv(message, COMMAND_BUFFER_SIZE, MPI_CHAR, parent, COMMAND_TAG);
        SendAll(message, COMMAND_BUFFER_SIZE, MPI_CHAR, children, COMMAND_TAG);
        
        if (strncmp(message, "end", 3) == 0) {
            for(unsigned int i = 0; i < children.size(); i++)
            {
                Recv(response, n_procs, MPI_INT, children[i]);
                for(int j = 0; j < n_procs; j++)
                {
                    if (response[j] != 0)
                    {
                        statistic[j] = response[j];
                    }
                }
            }
            Send(statistic, n_procs, MPI_INT, parent);
            break;
        }

        /* Receive image width from parent */
        Recv(&image_width, 1, MPI_INT, parent);

        /* Receive strip size from parent */
        Recv(&buffer_size, 1, MPI_INT, parent);

        /* If buffer size > 0, receive buffer as well */
        if (buffer_size > 0) {
            buffer = new int[buffer_size * image_width + 1];
            Recv(buffer, buffer_size * image_width, MPI_INT, parent);
        }

        /* Split strip & send it down */
        int *child_buffer_sizes = new int[children.size()]();
        std::vector<std::pair<int, int>> limits = GetChildBorders(buffer_size);
        int child_strip_size = 0;
        int strip_start = 0;
        for(unsigned int j = 0; j < children.size(); j++)
        {
            /* Send image width */
            Send(&image_width, 1, MPI_INT, children[j]);

            if (j < limits.size())
            {
                child_strip_size = limits[j].second - limits[j].first + 1;
            }
            child_buffer_sizes[j] = child_strip_size;
            
            /* Send strip size(in lines) */
            Send(&child_strip_size, 1, MPI_INT, children[j]);

            /* If strip size > 0, send buffer as well */
            if (child_strip_size > 0) {
                strip_start = limits[j].first * image_width;
                Send(&buffer[strip_start], child_strip_size * image_width, MPI_INT, children[j]);
            }
        }

        int active_children = 0;
        for(unsigned int k = 0; k < children.size(); k++)
        {
            if (child_buffer_sizes[k] > 0) {
                active_children ++;
            }
        }
        /* Receive edited buffers */
        for(int j = 0; j < (int )children.size(); j++)
        {
            /* if bufsize > 0, receive buffer from child */
            if (child_buffer_sizes[j] > 0) {
                processed_buffer = new int[child_buffer_sizes[j] * image_width]();
                Recv(processed_buffer, child_buffer_sizes[j] * image_width, MPI_INT, children[j]);

                if (active_children == 1 && j == 0) {
                    memcpy(buffer, processed_buffer, (child_buffer_sizes[j] *image_width) * sizeof(*buffer));
                }
                else if (j == 0) {
                    memcpy(buffer, processed_buffer, sizeof(*buffer)*(child_buffer_sizes[j]-1)*image_width);
                }
                else if (j == active_children - 1)
                {
                    int buf_start_idx = (limits[j].first + 1) * image_width;
                    int buf_stop_idx = buf_start_idx + (child_buffer_sizes[j] - 1) * image_width;
                    for(int k = buf_start_idx; k < buf_stop_idx; k++)
                    {
                        buffer[k] = processed_buffer[k - buf_start_idx + image_width];
                    }
                }
                else {
                    int buf_start_idx = (limits[j].first + 1) * image_width;
                    int buf_stop_idx = buf_start_idx + (child_buffer_sizes[j] - 2) * image_width;

                    for(int k = buf_start_idx; k < buf_stop_idx; k++) {
                        buffer[k] = processed_buffer[k - buf_start_idx + image_width];
                    }
                }
                delete[] processed_buffer;
            }
        }

        /* Send to parent */
        if (buffer_size > 0) {
            Send(buffer, buffer_size * image_width, MPI_INT, parent);
            delete[] buffer;
            buffer = NULL;
        }
    }
    delete[] statistic;
    statistic = NULL;
}


void Worker() {
    int processed_line_count = 0;
    while(1) {
        Recv(message, COMMAND_BUFFER_SIZE, MPI_CHAR, parent, COMMAND_TAG);

        if (strncmp(message, "end", 3) == 0) {
            statistic[rank] = processed_line_count;

            Send(statistic, n_procs, MPI_INT, parent);

            break;
        }

        /* Receive image width from parent */
        Recv(&image_width, 1, MPI_INT, parent);

        /* Receive strip size from parent */
        Recv(&buffer_size, 1, MPI_INT, parent);
        processed_line_count += buffer_size-2;

        /* If buffer size > 0, receive buffer as well */
        if (buffer_size > 0)
        {
            buffer = new int[buffer_size * image_width + 1];
            Recv(buffer, buffer_size * image_width, MPI_INT, parent);

            ProcessBuffer(buffer_size, image_width, buffer, std::string(message));

            Send(buffer, buffer_size * image_width, MPI_INT, parent);

            delete[] buffer;
            buffer = NULL;
        }
    }
    delete[] statistic;
    statistic = NULL;
}

std::vector<std::pair<int, int> > GetChildBorders(int buffer_size)
{
	std::vector<std::pair<int, int> > limits;

    int chunk_size = buffer_size / children.size();
    limits.push_back(std::make_pair(0, chunk_size - 1));

    for(unsigned int i = 1; i < children.size(); i++) {
        if (i == children.size() - 1) {
            limits.push_back(std::make_pair(limits[i-1].second - 1, buffer_size - 1));
        } else {
            limits.push_back(std::make_pair(limits[i-1].second - 1, limits[i-1].second - 2 + chunk_size));
        }
    }

	return limits;
}

void ProcessBuffer(int n, int m, int *array, std::string filter_name)
{
	int *old_array = new int[n * m]();

	memcpy(old_array, array, sizeof(*array)*(n*m));

	std::vector<int> filter;

    if (filter_mapping.count(filter_name) > 0) {
        filter = filter_mapping[filter_name];
    }
    else {
        ThrowError("Filter name not known.");
    }

    int *ptr = NULL;
	for(int i = 1; i < n - 1; i++) {
        ptr = &array[i * m + 1];
		for(int j = 1; j < m - 1; j++) {

			int sum = 0;				
			for(int ii = i - 1; ii < i + 2; ii++) {
				for (int jj = j - 1; jj < j + 2; jj++) {
					sum += old_array[ii * m + jj] * filter[3 * abs(i - 1 - ii) + abs(j - 1 - jj)];
				}	
			}
			*ptr = sum / filter[9];
			*ptr += filter[10];
			if (*ptr > 255)
				*ptr = 255;
			if (*ptr < 0)
                *ptr = 0;
            ptr++;
		}
	}
	delete[] old_array;
}