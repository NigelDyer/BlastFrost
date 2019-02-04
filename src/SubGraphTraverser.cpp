/*
 * GraphTraverser.cpp
 *
 *  Created on: 26 Feb 2018
 *      Author: nina
 */

#include "SubGraphTraverser.hpp"

using namespace std;


SubGraphTraverser::SubGraphTraverser(ColoredCDBG<UnitigData>& graph) : cdbg(graph) {
	cout << "GraphTraverser initialized!" << endl;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNC 2: Explore subgraph that contains queried sequence, i.e. all variants of a gene queried against the graph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SubGraphTraverser::extractSubGraph(const string& query, const int k, const int distance, string& outprefix, string& queryfile) {

	//ToDo: this should be a vector of Kmer!
	unordered_map<size_t,vector<Kmer>> map;

	//1) search for all k-mer hits inside of neighborhood defined by distance, save references to all unitigs covered
	//ToDo: we will only consider exact matches for now!
	const char *query_str = query.c_str();
	int startcounter = 0;

	for(KmerIterator it_km(query_str), it_km_end; it_km != it_km_end; ++it_km){
		startcounter += 1;
		UnitigColorMap<UnitigData> ucm = cdbg.find(it_km->first);


		ucm.dist = 0;
		ucm.len = ucm.size - Kmer::k + 1;
		//ucm.strand = true;


		if (!ucm.isEmpty) {

			const DataAccessor<UnitigData>* da = ucm.getData();
			const UnitigColors* uc = da->getUnitigColors(ucm);


			for (UnitigColors::const_iterator it = uc->begin(ucm); it != uc->end(); it.nextColor()) {
				const size_t color = it.getColorID();

				//check if color already in map
				//ToDo: do we need to do that?
				const std::unordered_map<size_t, vector<Kmer>>::iterator iter = map.find(color);
				//Kmer head = ucm.getUnitigHead();
				Kmer head = ucm.getMappedHead();

				if (iter == map.end()) {
					//cout << color << endl;
					//cout << startcounter << endl;

					vector<Kmer> newset;
					map.insert({color, newset});
					map[color].push_back(head);

				} else if (map[color].back() != head){ //check if this unitig has been found already
					map[color].push_back(head);
				}
			}
		}
	}

	//for each color, output last unitig
//	for(const auto& color : map){
//		vector<Kmer> current = color.second;
//		cout << "color: " << cdbg.getColorName(color.first) << endl;
//
//		cout << cdbg.find(current.back()).referenceUnitigToString() << endl;
//		cout << cdbg.find(current.front()).referenceUnitigToString() << endl;
//	}



		//Strategy: for each color, starting from the first unitig in the list, find all successors of the same color
		//if there is only one successor of the same color, add it to the path and pop it from the seed list if possible
		//otherwise, follow each successor path until a unitig from the seed list is found. Ignore the other path(s).
		//stop if the last unitig from the seed list is found

		//ToDo: Afterwards, for each color, we might want to extent the path at the end or the beginning...but how far?
		//We can first extend it following the longest color path? How can I anchor that?


	unordered_map<size_t,vector<Kmer>> all_paths;
	cout << "complete paths" << endl;
	for(const auto& color : map){
		//add this node to the collected path
		vector<Kmer> path;

		vector<Kmer> current = color.second;
		std::reverse(current.begin(),current.end());

		//cout << color.first << endl;
		while (! current.empty()){
			//cout << current.size() << endl;
			Kmer first = current.back();
			current.pop_back();

			path.push_back(first);

			//find all successors with the same color
			vector<Kmer> sameCol;

			UnitigColorMap<UnitigData> ucm = cdbg.find(first);


			for (const auto& successor : ucm.getSuccessors()){

				const DataAccessor<UnitigData>* da = successor.getData();
				const UnitigColors* uc = da->getUnitigColors(successor);
				//Kmer head = successor.getUnitigHead();
				Kmer head = successor.getMappedHead();

				for (UnitigColors::const_iterator it = uc->begin(successor); it != uc->end(); it.nextColor()) {
					if (it.getColorID() == color.first){
						sameCol.push_back(head);
					}
				}
			}

			if (sameCol.empty()){
				//there is no successor of the same color, this path stops here
				cout << "no successor!" << endl;

				//Note: the while expression will still try to extend other seed unitigs for this color!




			} else if (sameCol.size() == 1){
				if ((!current.empty()) && sameCol.back() == current.back()){
					//the single successor is already in the seed list
					//cout << "seed already in list" << endl;
				} else if ((!current.empty()) && ( std::find(path.begin(), path.end(), sameCol.back()) == path.end())){

					//the successor is not in the list, can fill a gap between seed unitigs
					current.push_back(sameCol.back());

				}
			} else {
				cout << "more than 2!" << endl;
			}
		}
		//std::reverse(path.begin(),path.end());
		all_paths[color.first] = path;
	}


	//different strategy: add seed unitigs to a new cdbg
	//add missing unitigs completing paths to new cdbg
	//in the end, the new cdbg should contain one CC
	//write new cdbg to gfa file!

	//SubGraphTraverser::pathLength(all_paths, query.size());

	unordered_map<size_t,std::string> paths = SubGraphTraverser::pathSequence(all_paths);
	SubGraphTraverser::printPaths(outprefix, queryfile, paths);

	SubGraphTraverser::testPath(all_paths);







	//take the length of the query as reference
	//previously, for each color, record the length of the prefix of the reference before the first seed hit
	//(record the length of the reference after the last seed hit as well?!)
	//for each color, extend the path of seeds in both ends until the length distance to the reference is minimized



	//at this point, assume we filled the prefix of the paths up already
	//so we can fill up its suffix until we reach the approximate length of the reference




	//to think about: output these unitigs as simple fasta for each color, then rebuild Bifrost graph with smaller k to get gfa file -> look at it in bandage

}





void SubGraphTraverser::pathLength(const unordered_map<size_t,vector<Kmer>>& all_paths, const int& ref_length) {
	//compute the length of each of the paths
		for (const auto& color : all_paths){
			vector<Kmer> current = color.second;
			int length = 0;
			cout << "color:" << color.first << endl;
			for(auto& head : current){
				int unitig_length = cdbg.find(head).referenceUnitigToString().size();

				if (length == 0){
					length = unitig_length;
				} else {
					length += unitig_length - 30;
				}
			}
			cout << length << endl;
		}
}



unordered_map<size_t,std::string> SubGraphTraverser::pathSequence(const unordered_map<size_t,vector<Kmer>>& all_paths) {
	unordered_map<size_t,std::string> all_paths_sequences;
	for (const auto& color : all_paths) {
		vector<Kmer> current = color.second;
		std::string result;

		bool first = true;
		for(auto& head : current){
			UnitigColorMap<UnitigData> ucm = cdbg.find(head);
			ucm.dist = 0;
			ucm.len = ucm.size - Kmer::k + 1;

			if (first){
				result += ucm.mappedSequenceToString();
				first = false;
			} else {
				string suff = ucm.mappedSequenceToString().substr(30);
				result += suff;
			}
		}
		all_paths_sequences[color.first] = result;
	}
	return all_paths_sequences;
}




void SubGraphTraverser::testPath(const unordered_map<size_t,vector<Kmer>>& all_paths) {

	//for each path, check if it continuous in the graph, i.e. the unitig overlaps are correct!
	for (const auto& color : all_paths) {
		vector<Kmer> current = color.second;
		string previous_suffix;
		string previous_seq;
		for (auto& head : current){

			UnitigColorMap<UnitigData> ucm = cdbg.find(head);

			ucm.dist = 0;
			ucm.len = ucm.size - Kmer::k + 1;
			//ucm.strand = true;

			string next = ucm.mappedSequenceToString();

			string next_prefix = next.substr(0,30);
			if (! previous_suffix.empty()){
				if (! previous_suffix.compare(next_prefix) == 0){
					cout << "ERROR path not continuous, suffix: " << previous_suffix << endl;
					cout << "ERROR path not continuous, prefix: " << next_prefix << endl;
				}
			}
			previous_seq = next;
			previous_suffix = next.substr(next.size() - 30);
		}
	}
}




void SubGraphTraverser::printPaths(string& outprefix, string& query, unordered_map<size_t,std::string>& paths) {
//output paths in fasta format, encode color name and paths length in header

	std::ofstream output(outprefix+"_"+query+"_subgraph.fasta",std::ios_base::out);

	for (const auto& color : paths){
		string color_name = cdbg.getColorName(color.first);
		output << ">" << color_name << "_len_" << color.second.length() << endl;
		output << color.second << endl;
	}

	output.close();
}



////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


void SubGraphTraverser::exploreSubgraph(const string& s) const {
	//ToDo: check graph structure of left and right border sequences! If we naively only look at last and first k-mer of the borders, we will miss everything with sequencing errors/mutations in there!

	Kmer old;
	for(KmerIterator it_km(s.c_str()), it_km_end; it_km != it_km_end; ++it_km){

		const const_UnitigColorMap<UnitigData> map = cdbg.find(it_km->first);

		cout << "Kmer: " << it_km->first.toString() << endl;

		if (! map.isEmpty) {

			bool new_unitig = false;

			//record if the previous kmer was present on the same unitig
			const Kmer head(map.getUnitigHead());

			if (head != old){

				cout << "new unitig!" << endl;
				cout << map.referenceUnitigToString() << endl;
				cout << map.size << endl;

				old = head;
				new_unitig = true;
			}

			if (new_unitig){
				//count number of predecessors and successors of this unitig
				int succ_count = 0;
				int pre_count = 0;

				for (const auto& successor : map.getSuccessors()) ++succ_count;
				for (const auto& predecessor : map.getPredecessors()) ++pre_count;

				cout << "Current unitig #successors: " << succ_count << endl;
				cout << "Current unitig #predecessors: " << pre_count << endl;
			}
		}
		else cout << "empty map!" << endl;
	}
}


/*
 * Find the unitig that corresponds to this string, and report all colors of this unitig
 */
vector<string> SubGraphTraverser::getColors(const string& u) const {

	vector<string> colors;

	const int k = cdbg.getK();
	const string kmer = u.substr(0,k);
	const char *cstr = kmer.c_str();

	const Kmer head(cstr);

	const const_UnitigColorMap<UnitigData> map = cdbg.find(head, true);

	if (!map.isEmpty) {

		const DataAccessor<UnitigData>* da = map.getData();
		const UnitigColors* uc = da->getUnitigColors(map);

		for (UnitigColors::const_iterator it = uc->begin(map); it != uc->end(); it.nextColor()) {

			colors.push_back(cdbg.getColorName(it.getColorID()));
		}

	}
	else cout << "Error, unitig not found." << endl;

	return colors;
}













