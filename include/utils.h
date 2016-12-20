#ifndef UTILS_H
#define UTILS_H

/****** some utility function ************/

template <typename K, typename V>
bool compare_pairs_second(const std::pair<K,V>& lhs, const std::pair<K,V>& rhs)
{
	return lhs.second < rhs.second;
}


#endif