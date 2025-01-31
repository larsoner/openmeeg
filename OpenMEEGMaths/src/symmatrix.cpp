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

#include <sstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "OpenMEEGMathsConfig.h"
#include "matrix.h"
#include "symmatrix.h"

namespace OpenMEEG {

    const SymMatrix& SymMatrix::operator=(const double d) {
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            data()[i] = d;
        return *this;
    }

    SymMatrix::SymMatrix(const Vector& v) {
        const size_t sz = v.size();
        nlin() = std::round((sqrt(1+8*sz)-1)/2);
        om_assert(size()==sz);
        value = v.value;
    }

    SymMatrix::SymMatrix(const Matrix& M): LinOp(M.nlin(),M.nlin(),SYMMETRIC,2),value(size()) {
        om_assert(nlin()==M.nlin());
        for (Index i=0; i<nlin(); ++i)
            for (Index j=i; j<nlin(); ++j)
                (*this)(i,j) = M(i,j);
    }

    void SymMatrix::set(const double x) {
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            data()[i] = x;
    }

    SymMatrix SymMatrix::operator*(const double x) const {
        SymMatrix C(nlin());
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            C.data()[i] = data()[i]*x;
        return C;
    }

    void SymMatrix::operator*=(const double x) {
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            data()[i] *= x;
    }

    Matrix SymMatrix::operator()(const Index i_start,const Index i_end,const Index j_start,const Index j_end) const {
        Matrix retMat(i_end-i_start+1,j_end-j_start+1);
        for (Index i=0; i<=i_end-i_start; ++i)
            for (Index j=0; j<=j_end-j_start; ++j)
                retMat(i,j) = (*this)(i_start+i,j_start+j);
        return retMat;
    }

    Matrix SymMatrix::submat(const Index istart,const Index isize,const Index jstart,const Index jsize) const {
        om_assert(istart+isize<=nlin());
        om_assert(jstart+jsize<=nlin());
        return (*this)(istart,istart+isize-1,jstart,jstart+jsize-1);
    }

    SymMatrix SymMatrix::submat(const Index istart,const Index iend) const {
        om_assert(iend>istart);
        const Index isize = iend-istart+1;
        om_assert(istart+isize<=nlin());

        SymMatrix mat(isize);
        for (Index i=istart; i<=iend; ++i)
            for (Index j=i; j<=iend; ++j)
                mat(i,j) = (*this)(i,j);

        return mat;
    }

    Matrix SymMatrix::operator*(const SymMatrix& m) const {
        om_assert(nlin()==m.nlin());
    // Workaround an MKL bug
    //#ifdef HAVE_BLAS
    #if defined(HAVE_BLAS) && !defined(USE_MKL)
        Matrix D(*this);
        Matrix B(m);
        Matrix C(nlin(),nlin());
        const BLAS_INT M = sizet_to_int(nlin());
        DSYMM(CblasLeft,CblasUpper,M,M,1.0,D.data(),M,B.data(),M,0.0,C.data(),M);
    #else
        Matrix C(nlin());
        for (Index j = 0; j<m.ncol(); ++j)
            for (Index i=0; i<ncol(); ++i) {
                C(i,j) = 0;
                for (Index k=0; k<ncol(); ++k)
                    C(i,j) += (*this)(i,k)*m(k,j);
            }
    #endif
        return C;
    }

    Matrix SymMatrix::operator*(const Matrix& B) const {
        om_assert(ncol()==B.nlin());
        Matrix C(nlin(),B.ncol());
    // Workaround an MKL bug
    //#ifdef HAVE_BLAS
    #if defined(HAVE_BLAS) && !defined(USE_MKL)
        Matrix D(*this);
        const BLAS_INT M = sizet_to_int(nlin());
        const BLAS_INT N = sizet_to_int(B.ncol());
        DSYMM(CblasLeft,CblasUpper,M,N,1.0,D.data(),M,B.data(),M,0.0,C.data(),M);
    #else
        for (Index j=0; j<B.ncol(); ++j)
            for (Index i=0; i<nlin(); ++i) {
                double sum = 0.0;
                for (Index k=0; k<i; ++k) {
                    C(k,j) += (*this)(i,k)*B(i,j);
                    sum += (*this)(i,k)*B(k,j);
                }
                C(i,j) = (*this)(i,i)*B(i,j)+sum;
            }
    #endif
        return C;
    }

    Matrix SymMatrix::solveLin(Matrix& RHS) const {
    om_assert(nlin()==RHS.nlin());
    #ifdef HAVE_LAPACK
        SymMatrix A(*this,DEEP_COPY);
        const BLAS_INT M = sizet_to_int(nlin());
        const BLAS_INT N = sizet_to_int(RHS.ncol());
        // LU
        BLAS_INT* pivots = new BLAS_INT[nlin()];
        int Info = 0;
        DSPTRF('U',M,A.data(),pivots,Info);
        // Solve the linear system AX=B
        DSPTRS('U',M,N,A.data(),pivots,RHS.data(),M,Info);
        om_assert(Info == 0);
        return RHS;
    #else
        std::cerr << "!!!!! solveLin not defined : Try a GMres !!!!!" << std::endl;
        exit(1);
    #endif
    }

    void SymMatrix::info() const {
        if (nlin()==0) {
            std::cout << "Matrix Empty" << std::endl;
            return;
        }

        std::cout << "Dimensions : " << nlin() << " x " << ncol() << std::endl;

        double minv = (*this)(0,0);
        double maxv = (*this)(0,0);
        Index mini = 0;
        Index maxi = 0;
        Index minj = 0;
        Index maxj = 0;

        for (Index i=0; i<nlin(); ++i)
            for (Index j=i; j<ncol(); ++j) {
                const double value = (*this)(i,j);
                if (minv>value) {
                    minv = value;
                    mini = i;
                    minj = j;
                } else if (maxv<value) {
                    maxv = value;
                    maxi = i;
                    maxj = j;
                }
            }

        std::cout << "Min Value : " << minv << " (" << mini << "," << minj << ")" << std::endl;
        std::cout << "Max Value : " << maxv << " (" << maxi << "," << maxj << ")" << std::endl;
        std::cout << "First Values" << std::endl;

        for (Index i=0; i<std::min(nlin(),5U); ++i) {
            for (Index j=i; j<std::min(ncol(),5U); ++j)
                std::cout << (*this)(i,j) << " " ;
            std::cout << std::endl ;
        }
    }

    // =======
    // = IOs =
    // =======

    void SymMatrix::load(const char* filename) {
        maths::ifstream ifs(filename);
        try {
            ifs >> maths::format(filename,maths::format::FromSuffix) >> *this;
        }
        catch (maths::Exception& e) {
            ifs >> *this;
        }
    }

    void SymMatrix::save(const char* filename) const {
        maths::ofstream ofs(filename);
        try {
            ofs << maths::format(filename,maths::format::FromSuffix) << *this;
        }
        catch (maths::Exception& e) {
            ofs << *this;
        }
    }
}
