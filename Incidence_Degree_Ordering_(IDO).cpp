#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm> // For std::sort, std::max, std::remove
#include <chrono>    // For high-resolution timing
#include <set>       // For calculating saturation degree (unique colors for DSATUR)
#include <numeric>   // For std::iota if needed (not directly used in final structure)

// Structure to represent a vertex
struct Vertex {
    int id;
    int degree;
    int color; // -1 indicates uncolored
    std::vector<int> neighbors;
    // This field is repurposed based on the algorithm:
    // For IDO: stores count of ALL colored neighbors
    // For DSATUR: stores saturation degree (count of UNIQUE colors of neighbors)
    // For RLF: stores count of neighbors in the 'U' set (forbidden for current color)
    int heuristic_value; 

    // Default constructor for vector resizing and initialization
    Vertex(int i = 0) : id(i), degree(0), color(-1), heuristic_value(0) {}
};

// Function to parse the DIMACS graph file format (including .col extension)
bool readGraphFile(const std::string& filename, std::vector<Vertex>& vertices, int& num_vertices, int& num_edges) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return false;
    }

    std::string line;
    bool problem_line_found = false;
    vertices.clear(); // Clear any previous graph data before reading a new one

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        char type;
        if (!(iss >> type)) { // Try to read the first character, skip empty lines
            continue;
        }

        if (type == 'c') {
            // Comment line, ignore
            continue;
        } else if (type == 'p') {
            std::string problem_type;
            if (!(iss >> problem_type >> num_vertices >> num_edges)) {
                std::cerr << "Error: Malformed 'p' line in '" << filename << "'" << std::endl;
                return false;
            }
            // Allow "edge" and "col" problem types for graph coloring benchmarks
            if (problem_type != "edge" && problem_type != "col") {
                std::cerr << "Error: Unsupported problem type '" << problem_type << "' in '" << filename << "'. Only 'edge' or 'col' types are supported by this parser." << std::endl;
                return false;
            }
            // Resize and initialize vertices. Using 1-indexing for convenience to match file format.
            vertices.resize(num_vertices + 1);
            for (int i = 1; i <= num_vertices; ++i) {
                vertices[i].id = i;
                vertices[i].color = -1; // Ensure it's uncolored
                vertices[i].degree = 0; // Ensure degree is reset
                vertices[i].neighbors.clear(); // Ensure neighbors list is clear
                vertices[i].heuristic_value = 0; // Ensure heuristic value is reset
            }
            problem_line_found = true;
        } else if (type == 'e') {
            if (!problem_line_found) {
                std::cerr << "Error: 'e' line found before 'p' line in '" << filename << "'" << std::endl;
                return false;
            }
            int u, v;
            if (!(iss >> u >> v)) {
                std::cerr << "Error: Malformed 'e' line in '" << filename << "'" << std::endl;
                return false;
            }
            // Add edges. Since it's an undirected graph, add to both adjacency lists.
            if (u > 0 && u <= num_vertices && v > 0 && v <= num_vertices) {
                vertices[u].neighbors.push_back(v);
                vertices[v].neighbors.push_back(u);
                vertices[u].degree++;
                vertices[v].degree++;
            } else {
                std::cerr << "Warning: Invalid vertex ID (" << u << ", " << v << ") in edge in '" << filename << "'. Max vertex ID is " << num_vertices << std::endl;
            }
        }
    }
    file.close();

    if (!problem_line_found) {
        std::cerr << "Error: No 'p' line found in '" << filename << "'" << std::endl;
        return false;
    }
    return true;
}

// Function to check if a color is valid for a vertex
bool isColorValid(const Vertex& current_vertex, int color, const std::vector<Vertex>& all_vertices) {
    for (int neighbor_id : current_vertex.neighbors) {
        // Check if the neighbor is colored and has the same color
        if (all_vertices[neighbor_id].color == color) {
            return false;
        }
    }
    return true;
}

// Helper function to find the uncolored vertex with the largest degree
// Returns a pointer to the vertex, or nullptr if no uncolored vertices remain.
Vertex* find_max_degree_uncolored_vertex(std::vector<Vertex>& vertices, int num_vertices) {
    Vertex* max_degree_vertex = nullptr;
    int max_degree = -1;

    for (int i = 1; i <= num_vertices; ++i) {
        if (vertices[i].color == -1) { // If uncolored
            if (max_degree_vertex == nullptr || vertices[i].degree > max_degree) {
                max_degree = vertices[i].degree;
                max_degree_vertex = &vertices[i];
            }
        }
    }
    return max_degree_vertex;
}

// Common logic for greedy coloring algorithms (IDO and DSATUR)
// Heuristic calculation and selection varies by algorithm
// Returns the total number of colors used
int generic_greedy_coloring(std::vector<Vertex>& vertices, int num_vertices, const std::string& alg_name) {
    std::vector<int> color_palette; // Stores the distinct colors used (e.g., 0, 1, 2)
    int next_available_color_idx = 0; // The next color to try if no existing color fits

    // Create a list of pointers to uncolored vertices for efficient sorting and removal.
    std::vector<Vertex*> uncolored_vertices_ptrs;
    uncolored_vertices_ptrs.reserve(num_vertices); // Pre-allocate memory

    // Initialize all vertices to uncolored and populate the pointers vector
    for (int i = 1; i <= num_vertices; ++i) {
        vertices[i].color = -1; // Ensure a clean state for coloring for this run
        uncolored_vertices_ptrs.push_back(&vertices[i]);
    }

    // Handle empty graph case
    if (uncolored_vertices_ptrs.empty()) {
        return 0; // No vertices, no colors needed
    }

    // Step 2 (for IDO/DSATUR): Select the uncolored vertex that has the largest degree.
    // This initial sort applies to both algorithms for the very first vertex.
    std::sort(uncolored_vertices_ptrs.begin(), uncolored_vertices_ptrs.end(),
              [](const Vertex* a, const Vertex* b) {
                  return a->degree > b->degree;
              });

    // Color the first selected vertex (highest degree) with the first color (0)
    Vertex* initial_vertex = uncolored_vertices_ptrs[0];
    initial_vertex->color = next_available_color_idx;
    color_palette.push_back(next_available_color_idx);
    next_available_color_idx++;

    // Remove the colored vertex from the uncolored list of pointers
    uncolored_vertices_ptrs.erase(
        std::remove(uncolored_vertices_ptrs.begin(), uncolored_vertices_ptrs.end(), initial_vertex),
        uncolored_vertices_ptrs.end());


    // Main coloring loop (Step 5: If uncolored vertex exists, return to step 3)
    while (!uncolored_vertices_ptrs.empty()) {
        // Step 3: Calculate the heuristic value and find the best vertex for this iteration.
        Vertex* best_vertex_for_this_iteration = nullptr;

        for (Vertex* v_ptr : uncolored_vertices_ptrs) {
            if (alg_name == "DSATUR") {
                std::set<int> distinct_neighbor_colors;
                for (int neighbor_id : v_ptr->neighbors) {
                    if (vertices[neighbor_id].color != -1) { // If neighbor is colored
                        distinct_neighbor_colors.insert(vertices[neighbor_id].color);
                    }
                }
                v_ptr->heuristic_value = distinct_neighbor_colors.size();
            } else { // Assume IDO if not DSATUR
                v_ptr->heuristic_value = 0; // Reset count for current iteration
                for (int neighbor_id : v_ptr->neighbors) {
                    if (vertices[neighbor_id].color != -1) { // If neighbor is colored
                        v_ptr->heuristic_value++;
                    }
                }
            }

            // Select the uncolored vertex with the maximum heuristic_value.
            // If tied, select the one with the largest degree among them.
            if (best_vertex_for_this_iteration == nullptr ||
                v_ptr->heuristic_value > best_vertex_for_this_iteration->heuristic_value ||
                (v_ptr->heuristic_value == best_vertex_for_this_iteration->heuristic_value && v_ptr->degree > best_vertex_for_this_iteration->degree)) {
                
                best_vertex_for_this_iteration = v_ptr;
            }
        }

        if (best_vertex_for_this_iteration == nullptr) {
            std::cerr << "Warning [" << alg_name << "]: No best vertex found while uncolored_vertices_ptrs is not empty. Breaking loop." << std::endl;
            break;
        }

        // Step 4: Try to color the selected vertex with existing colors.
        int chosen_color = -1;
        for (int c : color_palette) {
            if (isColorValid(*best_vertex_for_this_iteration, c, vertices)) {
                chosen_color = c;
                break; // Found a valid existing color
            }
        }

        if (chosen_color == -1) {
            // No existing color is suitable, assign a new color
            chosen_color = next_available_color_idx;
            color_palette.push_back(chosen_color);
            next_available_color_idx++;
        }

        best_vertex_for_this_iteration->color = chosen_color;

        // Remove the colored vertex from the uncolored list of pointers
        uncolored_vertices_ptrs.erase(
            std::remove(uncolored_vertices_ptrs.begin(), uncolored_vertices_ptrs.end(), best_vertex_for_this_iteration),
            uncolored_vertices_ptrs.end());
    }

    return next_available_color_idx; // Return the total number of colors used
}

// Wrapper for IDO
int IDO_coloring(std::vector<Vertex>& vertices, int num_vertices) {
    return generic_greedy_coloring(vertices, num_vertices, "IDO");
}

// Wrapper for DSATUR
int DSATUR_coloring(std::vector<Vertex>& vertices, int num_vertices) {
    return generic_greedy_coloring(vertices, num_vertices, "DSATUR");
}

// Implementation of the Recursive Largest First Algorithm (RLF)
int RLF_coloring(std::vector<Vertex>& vertices, int num_vertices) {
    int current_color = 0;
    int total_colored_vertices = 0;

    // Reset all vertex colors at the start of RLF run
    for (int i = 1; i <= num_vertices; ++i) {
        vertices[i].color = -1;
    }

    // Outer loop: Iterate through colors until all vertices are colored
    while (total_colored_vertices < num_vertices) {
        // Step 1: Select the uncolored vertex which has the largest degree.
        Vertex* v_i = find_max_degree_uncolored_vertex(vertices, num_vertices);

        if (v_i == nullptr) { // Should not happen if total_colored_vertices < num_vertices
            std::cerr << "Warning [RLF]: No uncolored vertex found unexpectedly. Breaking loop." << std::endl;
            break;
        }

        // Step 2: The selected vertex is colored with active color.
        v_i->color = current_color;
        total_colored_vertices++;
        
        // U set: Neighbors of the current color class members (cannot take active color)
        // V_prime: Uncolored vertices NOT adjacent to any in the current color class (can potentially take active color)
        std::vector<bool> forbidden_for_current_color(num_vertices + 1, false); 
        
        // Initialize forbidden set U with neighbors of v_i
        for (int neighbor_id : v_i->neighbors) {
            forbidden_for_current_color[neighbor_id] = true;
        }

        // Inner loop: Recursively select uncolored vertices for the current color class
        while (true) {
            Vertex* v_j_candidate = nullptr; // The next vertex to add to current color class
            int max_adj_in_U = -1;

            // Iterate through all vertices to find candidates for V' and calculate heuristic
            for (int k = 1; k <= num_vertices; ++k) {
                if (vertices[k].color == -1 && !forbidden_for_current_color[k]) { // Is uncolored AND not forbidden (in V')
                    // Calculate number of adjacent vertices which are in the set of U for every vertex in set of V'
                    int current_adj_in_U = 0;
                    for (int neighbor_of_vk : vertices[k].neighbors) {
                        if (forbidden_for_current_color[neighbor_of_vk]) { // If neighbor is in U (forbidden set)
                            current_adj_in_U++;
                        }
                    }
                    vertices[k].heuristic_value = current_adj_in_U; // Store for selection

                    // Select the uncolored vertex whose has maximum adjacent vertices (which are in the set of U) in the set of V'
                    // If more than one vertex provide this condition, the vertex which has the largest degree among them is selected.
                    if (v_j_candidate == nullptr || 
                        vertices[k].heuristic_value > max_adj_in_U ||
                        (vertices[k].heuristic_value == max_adj_in_U && vertices[k].degree > v_j_candidate->degree)) {
                        
                        max_adj_in_U = vertices[k].heuristic_value;
                        v_j_candidate = &vertices[k];
                    }
                }
            }

            if (v_j_candidate == nullptr) {
                // Set V' is empty or no more eligible vertices for this color
                break; // Move to Step 3 (next color)
            }

            // The selected vertex is colored with active color.
            v_j_candidate->color = current_color;
            total_colored_vertices++;

            // Update forbidden set U: Add adjacent vertices of the colored vertex to U
            for (int neighbor_id : v_j_candidate->neighbors) {
                forbidden_for_current_color[neighbor_id] = true;
            }
            // The colored vertex and its neighbors (which are now in U) are implicitly handled.
            // No explicit deletion from V' is needed if we re-evaluate V' candidates in each iteration.
        }
        
        // Step 3: If uncolored vertex exists, next color in the color set is selected as active.
        current_color++; // Prepare for the next color class
    }

    // After loop, current_color holds the total number of colors used (since it increments one last time).
    return current_color; 
}


int main() {
    // Define the folder where the graph files are located
    const std::string graph_folder = "DIMACS_Graphs_Instances/";
    const std::string log_filename = "results.log";

    std::vector<std::string> filenames = {
        "dsjc250.5.col",
        "dsjc500.1.col",
        // "dsjc500.5.col",
        // "dsjc500.9.col",
        // "dsjc1000.1.col",
        // "r250.5.col",
        // "r1000.1c.col",
        // "r1000.5.col",
        // "dsjr500.1c.col",
        // "dsjr500.5.col",
        // "le450_25c.col",
        // "le450_25d.col",
        // "flat300_28_0.col",
        // "flat1000_50_0.col",
        // "flat1000_60_0.col",
        // "flat1000_76_0.col",
        // "latin_square.col",
        // "C2000.5.col",
        // "C4000.5.col",
    };

    std::ofstream log_file(log_filename, std::ios_base::app);
    if (!log_file.is_open()) {
        std::cerr << "Error: Could not open log file '" << log_filename << "'" << std::endl;
        return 1; 
    }

    log_file << "--- Graph Coloring Algorithms Comparison Session Start: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << " ---" << std::endl;
    std::cout << "--- Graph Coloring Algorithms Comparison ---" << std::endl;

    // A single vector to reuse for graph data to save memory,
    // cleared and resized for each new graph.
    std::vector<Vertex> vertices_storage;
    int num_vertices_current = 0;
    int num_edges_current = 0;

    for (const std::string& filename : filenames) {
        // Construct the full path to the graph file
        std::string full_path_filename = graph_folder + filename;

        std::cout << "\nProcessing graph file: '" << full_path_filename << "'" << std::endl;
        log_file << "\nProcessing graph file: '" << full_path_filename << "'" << std::endl;

        // Attempt to read the graph file
        if (!readGraphFile(full_path_filename, vertices_storage, num_vertices_current, num_edges_current)) {
            std::cerr << "Failed to read graph from '" << full_path_filename << "'. Skipping." << std::endl;
            log_file << "Failed to read graph from '" << full_path_filename << "'. Skipping." << std::endl;
            continue; // Move to the next file in the list
        }

        std::cout << "  Graph loaded: " << num_vertices_current << " vertices, " << num_edges_current << " edges." << std::endl;
        log_file << "  Graph loaded: " << num_vertices_current << " vertices, " << num_edges_current << " edges." << std::endl;

        // --- Run IDO Algorithm ---
        std::cout << "\n  Algorithm: IDO" << std::endl;
        log_file << "\n  Algorithm: IDO" << std::endl;
        auto start_time_ido = std::chrono::high_resolution_clock::now();
        int colors_used_ido = IDO_coloring(vertices_storage, num_vertices_current);
        auto end_time_ido = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_milliseconds_ido = end_time_ido - start_time_ido;

        std::cout << "    Colors Used: " << colors_used_ido << std::endl;
        std::cout << "    CPU Time:    " << elapsed_milliseconds_ido.count() << " ms" << std::endl;
        log_file << "    Colors Used: " << colors_used_ido << std::endl;
        log_file << "    CPU Time:    " << elapsed_milliseconds_ido.count() << " ms" << std::endl;

        // --- Run DSATUR Algorithm ---
        std::cout << "\n  Algorithm: DSATUR" << std::endl;
        log_file << "\n  Algorithm: DSATUR" << std::endl;
        auto start_time_dsatur = std::chrono::high_resolution_clock::now();
        int colors_used_dsatur = DSATUR_coloring(vertices_storage, num_vertices_current);
        auto end_time_dsatur = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_milliseconds_dsatur = end_time_dsatur - start_time_dsatur;

        std::cout << "    Colors Used: " << colors_used_dsatur << std::endl;
        std::cout << "    CPU Time:    " << elapsed_milliseconds_dsatur.count() << " ms" << std::endl;
        log_file << "    Colors Used: " << colors_used_dsatur << std::endl;
        log_file << "    CPU Time:    " << elapsed_milliseconds_dsatur.count() << " ms" << std::endl;

        // --- Run RLF Algorithm ---
        std::cout << "\n  Algorithm: RLF" << std::endl;
        log_file << "\n  Algorithm: RLF" << std::endl;
        auto start_time_rlf = std::chrono::high_resolution_clock::now();
        int colors_used_rlf = RLF_coloring(vertices_storage, num_vertices_current);
        auto end_time_rlf = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_milliseconds_rlf = end_time_rlf - start_time_rlf;

        std::cout << "    Colors Used: " << colors_used_rlf << std::endl;
        std::cout << "    CPU Time:    " << elapsed_milliseconds_rlf.count() << " ms" << std::endl;
        log_file << "    Colors Used: " << colors_used_rlf << std::endl;
        log_file << "    CPU Time:    " << elapsed_milliseconds_rlf.count() << " ms" << std::endl;
    }

    // Final message to log file and console
    log_file << "\n--- All specified files processed ---" << std::endl;
    log_file << "--- Graph Coloring Algorithms Comparison Session End: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << " ---" << std::endl;
    std::cout << "\n--- All specified files processed ---" << std::endl;

    log_file.close();

    return 0;
}