/*
Project Name : OpenMEEG

© INRIA and ENPC (contributors: Geoffray ADDE, Maureen CLERC, Alexandre
GRAMFORT, Renaud KERIVEN, Jan KYBIC, Perrine LANDREAU, Théodore PAPADOPOULO,
Emmanuel OLIVI
Maureen.Clerc.AT.inria.fr, keriven.AT.certis.enpc.fr,
kybic.AT.fel.cvut.cz, papadop.AT.inria.fr)

The OpenMEEG software is a C++ package for solving the forward/inverse
problems of electroencephalography and magnetoencephalography.

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's authors,  the holders of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#include <iostream>
#include <range.h>
#include <block_matrix.h>
#include <symm_block_matrix.h>

int main() {

    using namespace OpenMEEG::maths;

    Ranges row_ranges;
    Ranges col_ranges;

    row_ranges.push_back(Range(1,4));
    row_ranges.push_back(Range(5,6));
    row_ranges.push_back(Range(7,9));

    col_ranges.push_back(Range(1,2));
    col_ranges.push_back(Range(5,9));

    BlockMatrix bm(10,10);
    bm.set_blocks(row_ranges,col_ranges);
    bm.info();

    std::cout << bm << std::endl;

    // section SymMatrix

    SymmetricBlockMatrix sbm(10);
    sbm.set_blocks(row_ranges);
    sbm.info();
}
